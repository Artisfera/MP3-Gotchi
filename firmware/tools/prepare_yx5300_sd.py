#!/usr/bin/env python3
"""
Friendly SD card preparation helper for MP3-Gotchi / YX5300 modules.

Default flow:
  1. Put songs in firmware/tools/songs.
  2. Run: python firmware/tools/prepare_yx5300_sd.py
  3. Pick the SD card drive from the numbered list.
  4. Answer simple y/n questions.

The script copies audio files in alphabetical order. YX5300-style modules often
play files by FAT directory order, so copying to an empty SD card matters.
"""

from __future__ import annotations

import ctypes
import json
import os
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


AUDIO_EXTENSIONS = {".mp3", ".wav", ".wma"}
REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_SOURCE = REPO_ROOT / "firmware" / "tools" / "songs"


@dataclass
class DriveInfo:
    root: Path
    label: str
    drive_type: str
    total_bytes: int
    free_bytes: int


def human_size(size: int) -> str:
    units = ["B", "KB", "MB", "GB", "TB"]
    value = float(size)
    for unit in units:
        if value < 1024 or unit == units[-1]:
            return f"{value:.1f} {unit}" if unit != "B" else f"{int(value)} B"
        value /= 1024
    return f"{size} B"


def ask_yes_no(question: str, default: bool = False) -> bool:
    suffix = "Y/n" if default else "y/N"
    while True:
        answer = input(f"{question} [{suffix}]: ").strip().lower()
        if not answer:
            return default
        if answer in {"y", "yes", "t", "tak"}:
            return True
        if answer in {"n", "no", "nie"}:
            return False
        print("Please answer y or n.")


def ask_number(question: str, minimum: int, maximum: int) -> int:
    while True:
        answer = input(f"{question} ({minimum}-{maximum}): ").strip()
        try:
            value = int(answer)
        except ValueError:
            print("Please type a number from the list.")
            continue
        if minimum <= value <= maximum:
            return value
        print("That number is outside the list.")


def get_windows_drives() -> list[DriveInfo]:
    kernel32 = ctypes.windll.kernel32
    bitmask = kernel32.GetLogicalDrives()
    drive_types = {
        0: "Unknown",
        1: "No root",
        2: "Removable",
        3: "Fixed",
        4: "Network",
        5: "CD/DVD",
        6: "RAM disk",
    }
    drives: list[DriveInfo] = []

    for index in range(26):
        if not (bitmask & (1 << index)):
            continue

        letter = chr(ord("A") + index)
        root_text = f"{letter}:\\"
        root = Path(root_text)
        drive_type_code = kernel32.GetDriveTypeW(ctypes.c_wchar_p(root_text))
        drive_type = drive_types.get(drive_type_code, "Unknown")

        if drive_type_code in {0, 1, 4, 5, 6}:
            continue

        print(f"Checking drive {root_text}...", flush=True)
        details = get_windows_drive_details(root_text)
        if details is None:
            label = "(not ready)"
            total_bytes = 0
            free_bytes = 0
        else:
            label = details["label"] or "(no label)"
            total_bytes = int(details["total_bytes"])
            free_bytes = int(details["free_bytes"])

        drives.append(
            DriveInfo(
                root=root,
                label=label,
                drive_type=drive_type,
                total_bytes=total_bytes,
                free_bytes=free_bytes,
            )
        )

    return drives


def get_windows_drive_details(root_text: str) -> dict[str, object] | None:
    probe = r"""
import ctypes, json, sys
root = sys.argv[1]
k = ctypes.windll.kernel32
free = ctypes.c_ulonglong(0)
total = ctypes.c_ulonglong(0)
total_free = ctypes.c_ulonglong(0)
if not k.GetDiskFreeSpaceExW(ctypes.c_wchar_p(root), ctypes.byref(free), ctypes.byref(total), ctypes.byref(total_free)):
    raise SystemExit(2)
label_buffer = ctypes.create_unicode_buffer(261)
fs_buffer = ctypes.create_unicode_buffer(261)
serial = ctypes.c_ulong(0)
max_component = ctypes.c_ulong(0)
flags = ctypes.c_ulong(0)
label = ""
if k.GetVolumeInformationW(ctypes.c_wchar_p(root), label_buffer, len(label_buffer), ctypes.byref(serial), ctypes.byref(max_component), ctypes.byref(flags), fs_buffer, len(fs_buffer)):
    label = label_buffer.value
print(json.dumps({"label": label, "total_bytes": total.value, "free_bytes": free.value}))
"""
    try:
        result = subprocess.run(
            [sys.executable, "-c", probe, root_text],
            capture_output=True,
            text=True,
            timeout=2.0,
            check=True,
        )
    except (subprocess.SubprocessError, OSError):
        return None

    try:
        return json.loads(result.stdout.strip())
    except json.JSONDecodeError:
        return None


def get_drives() -> list[DriveInfo]:
    if os.name == "nt":
        return get_windows_drives()

    drives: list[DriveInfo] = []
    for base in [Path("/media"), Path("/mnt"), Path("/Volumes")]:
        if not base.exists():
            continue
        for mount in base.iterdir():
            if not mount.is_dir():
                continue
            usage = shutil.disk_usage(mount)
            drives.append(
                DriveInfo(
                    root=mount,
                    label=mount.name,
                    drive_type="Mount",
                    total_bytes=usage.total,
                    free_bytes=usage.free,
                )
            )
    return drives


def choose_source() -> Path:
    print("\nSongs folder")
    print(f"Default: {DEFAULT_SOURCE}")
    if DEFAULT_SOURCE.exists():
        return DEFAULT_SOURCE

    print("The default firmware/tools/songs folder does not exist yet.")
    custom = input("Type another songs folder path, or press Enter to stop: ").strip().strip('"')
    if not custom:
        raise SystemExit("Create firmware/tools/songs, put audio files there, then run this helper again.")
    return Path(custom).expanduser().resolve()


def list_audio_files(source: Path) -> list[Path]:
    if not source.exists() or not source.is_dir():
        raise SystemExit(f"Songs folder not found: {source}")

    files = [
        path
        for path in source.iterdir()
        if path.is_file() and path.suffix.lower() in AUDIO_EXTENSIONS
    ]
    files.sort(key=lambda path: path.name.lower())
    if not files:
        raise SystemExit(f"No .mp3, .wav, or .wma files found in: {source}")
    return files


def choose_drive(drives: list[DriveInfo]) -> DriveInfo:
    if not drives:
        raise SystemExit("No drives found. Insert the SD card and run this helper again.")

    print("\nAvailable drives")
    for number, drive in enumerate(drives, start=1):
        label = f"{drive.root} - {drive.label}"
        size = f"{human_size(drive.total_bytes)} total, {human_size(drive.free_bytes)} free"
        print(f"{number}. {label} [{drive.drive_type}] ({size})")

    choice = ask_number("Choose the SD card drive", 1, len(drives))
    selected = drives[choice - 1]

    print(f"\nSelected: {selected.root} - {selected.label}")
    print(f"Type: {selected.drive_type}")
    print(f"Size: {human_size(selected.total_bytes)} total, {human_size(selected.free_bytes)} free")

    if selected.total_bytes == 0:
        raise SystemExit("This drive did not answer in time. Reinsert the SD card and run the helper again.")

    if selected.drive_type not in {"Removable", "Mount"}:
        print("\nWarning: this does not look like a removable SD card.")
        if not ask_yes_no("Are you sure this is the correct destination?", False):
            raise SystemExit("Cancelled.")

    return selected


def visible_items(path: Path) -> list[Path]:
    return [item for item in path.iterdir() if item.name not in {"System Volume Information"}]


def clear_destination(destination: Path) -> None:
    for item in visible_items(destination):
        if item.is_dir():
            shutil.rmtree(item)
        else:
            item.unlink()


def copy_songs(files: list[Path], destination: Path, prefix_numeric: bool) -> None:
    print("\nCopy order")
    for index, source in enumerate(files, start=1):
        if prefix_numeric:
            target_name = f"{index:03d}-{source.name}"
        else:
            target_name = source.name
        print(f"{index:03d}: {target_name}")

    if not ask_yes_no("\nStart copying now?", True):
        raise SystemExit("Cancelled.")

    for index, source in enumerate(files, start=1):
        target_name = f"{index:03d}-{source.name}" if prefix_numeric else source.name
        shutil.copy2(source, destination / target_name)

    print(f"\nDone. Copied {len(files)} files to {destination}.")


def main() -> None:
    print("MP3-Gotchi SD card helper")
    print("========================")
    print("This tool copies songs to an SD card in alphabetical order.")

    source = choose_source()
    files = list_audio_files(source)
    total_size = sum(path.stat().st_size for path in files)

    print(f"\nFound {len(files)} audio files in {source}")
    print(f"Total size: {human_size(total_size)}")

    drives = get_drives()
    selected = choose_drive(drives)
    destination = selected.root

    if selected.free_bytes > 0 and total_size > selected.free_bytes:
        raise SystemExit("The selected drive does not have enough free space.")

    existing = visible_items(destination)
    if existing:
        print(f"\nThe destination is not empty: {destination}")
        print("For best YX5300 ordering, the SD card should be empty before copying.")
        if ask_yes_no("Erase existing visible files/folders on this destination?", False):
            clear_destination(destination)
        else:
            raise SystemExit("Cancelled. Please use an empty SD card or allow clearing it.")

    prefix_numeric = ask_yes_no(
        "Add 001-, 002-, 003- prefixes for extra compatibility?",
        False,
    )
    copy_songs(files, destination, prefix_numeric)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nCancelled.")
        sys.exit(1)
