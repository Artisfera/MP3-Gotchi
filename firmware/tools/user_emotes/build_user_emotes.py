#!/usr/bin/env python3
"""
Build MP3-Gotchi user emotes from full-screen PNG layers.
PNG is only the host-side source format; firmware uses generated sparse RGB565 data.

Default workflow:
  1. Put PNG layers in firmware/user_emotes/my_emote/
  2. Run: py firmware/tools/user_emotes/build_user_emotes.py
  3. Open the latest firmware sketch and upload it.

Black pixels and transparent pixels are treated as empty space.
"""

from __future__ import annotations

import argparse
import json
import re
from dataclasses import dataclass
from pathlib import Path


DISPLAY_W = 96
DISPLAY_H = 64
GENERATED_PNG_NAMES = {"emote.png", "emote_preview.png"}
CONFIG_NAME = "emotes.json"


@dataclass
class EmoteBuild:
    folder: Path
    key: str
    name: str
    weight: float
    duration_ms: int


def parse_args() -> argparse.Namespace:
    root = repo_root()
    parser = argparse.ArgumentParser(description="Build MP3-Gotchi user emotes from PNG layer folders.")
    parser.add_argument("--source", type=Path, default=root / "firmware" / "user_emotes", help="Folder with emote PNG folders.")
    parser.add_argument("--output", type=Path, default=None, help="Output UserEmotes.h path. Defaults to latest firmware/src sketch.")
    parser.add_argument("--all", action="store_true", help="Build every detected emote folder without asking which ones.")
    parser.add_argument("--yes", action="store_true", help="Use saved/default names, weights, and durations without prompts.")
    parser.add_argument("--default-weight", type=float, default=1.0)
    parser.add_argument("--default-duration", type=int, default=2400)
    parser.add_argument("--black-threshold", type=int, default=8, help="RGB values up to this threshold are treated as transparent.")
    parser.add_argument("--alpha-threshold", type=int, default=16, help="Alpha values below this threshold are treated as transparent.")
    return parser.parse_args()


def repo_root() -> Path:
    return Path(__file__).resolve().parents[3]


def latest_sketch_dir() -> Path:
    src = repo_root() / "firmware" / "src"
    candidates: list[tuple[tuple[int, int, int], Path]] = []
    for path in src.glob("mp3gotchi-*"):
        if not path.is_dir():
            continue
        match = re.match(r"mp3gotchi-(\d+)\.(\d+)\.(\d+)$", path.name)
        if match:
            candidates.append(((int(match.group(1)), int(match.group(2)), int(match.group(3))), path))
    if not candidates:
        raise SystemExit("No firmware/src/mp3gotchi-x.y.z sketch folder found.")
    return max(candidates, key=lambda item: item[0])[1]


def ask(prompt: str, default: str | None = None) -> str:
    suffix = f" [{default}]" if default not in (None, "") else ""
    value = input(f"{prompt}{suffix}: ").strip()
    if not value and default is not None:
        return default
    return value


def ask_yes_no(prompt: str, default: bool = True) -> bool:
    suffix = "Y/n" if default else "y/N"
    while True:
        value = input(f"{prompt} ({suffix}): ").strip().lower()
        if not value:
            return default
        if value in {"y", "yes", "t", "tak"}:
            return True
        if value in {"n", "no", "nie"}:
            return False
        print("Please type y or n.")


def ask_float(prompt: str, default: float) -> float:
    while True:
        value = ask(prompt, str(default))
        try:
            parsed = float(value.replace(",", "."))
            if parsed > 0:
                return parsed
        except ValueError:
            pass
        print("Use a number above 0, for example 1 or 0.5.")


def ask_int(prompt: str, default: int, minimum: int = 1) -> int:
    while True:
        value = ask(prompt, str(default))
        try:
            parsed = int(value)
            if parsed >= minimum:
                return parsed
        except ValueError:
            pass
        print(f"Use a whole number, minimum {minimum}.")


def png_layers(folder: Path) -> list[Path]:
    return sorted(
        [
            path
            for path in folder.glob("*.png")
            if path.name.lower() not in GENERATED_PNG_NAMES and path.is_file()
        ],
        key=lambda path: path.name.lower(),
    )


def find_emote_folders(source: Path) -> list[Path]:
    found: list[Path] = []
    if png_layers(source):
        found.append(source)
    for child in sorted(source.iterdir(), key=lambda path: path.name.lower()) if source.exists() else []:
        if child.is_dir() and png_layers(child):
            found.append(child)
    return found


def load_config(source: Path) -> dict:
    path = source / CONFIG_NAME
    if not path.exists():
        return {}
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        print(f"Warning: could not read {path}. Defaults will be used.")
        return {}


def save_config(source: Path, config: dict) -> None:
    source.mkdir(parents=True, exist_ok=True)
    (source / CONFIG_NAME).write_text(json.dumps(config, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


def folder_key(source: Path, folder: Path) -> str:
    try:
        rel = folder.relative_to(source)
    except ValueError:
        return folder.name
    return "." if str(rel) == "." else rel.as_posix()


def clean_name(value: str, fallback: str) -> str:
    value = value.strip()
    if not value:
        value = fallback
    return re.sub(r"[^A-Za-z0-9 _.-]+", "", value).strip() or fallback


def symbol_name(name: str, index: int) -> str:
    clean = re.sub(r"[^A-Za-z0-9]+", "_", name).strip("_").upper()
    if not clean:
        clean = f"EMOTE_{index + 1}"
    if clean[0].isdigit():
        clean = f"EMOTE_{clean}"
    return f"USER_EMOTE_{clean}"


def choose_folders(folders: list[Path], args: argparse.Namespace) -> list[Path]:
    if args.all or args.yes:
        return folders

    print("\nDetected emotes")
    for index, folder in enumerate(folders, start=1):
        layers = ", ".join(path.name for path in png_layers(folder))
        print(f"{index}. {folder} ({layers})")

    choice = ask("\nChoose emotes: A for all, or numbers like 1,3", "A").lower()
    if choice in {"a", "all", "*"}:
        return folders

    selected: list[Path] = []
    for part in choice.split(","):
        part = part.strip()
        if not part:
            continue
        if not part.isdigit():
            print(f"Skipping invalid choice: {part}")
            continue
        index = int(part)
        if 1 <= index <= len(folders):
            selected.append(folders[index - 1])
        else:
            print(f"Skipping out-of-range choice: {part}")
    return selected


def collect_builds(source: Path, folders: list[Path], args: argparse.Namespace) -> list[EmoteBuild]:
    config = load_config(source)
    builds: list[EmoteBuild] = []

    for folder in folders:
        key = folder_key(source, folder)
        meta = config.get(key, {})
        default_name = clean_name(str(meta.get("name", folder.name)), folder.name)
        default_weight = float(meta.get("weight", args.default_weight))
        default_duration = int(meta.get("duration_ms", args.default_duration))

        if args.yes:
            name = default_name
            weight = default_weight
            duration = default_duration
        else:
            print(f"\nEmote: {folder}")
            name = clean_name(ask("Display name", default_name), default_name)
            weight = ask_float("Weight/chance inside the emote pool", default_weight)
            duration = ask_int("Show duration in milliseconds", default_duration, 100)

        config[key] = {"name": name, "weight": weight, "duration_ms": duration}
        builds.append(EmoteBuild(folder=folder, key=key, name=name, weight=weight, duration_ms=duration))

    save_config(source, config)
    return builds


def rgb565(r: int, g: int, b: int) -> int:
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def compose_emote(build: EmoteBuild, black_threshold: int, alpha_threshold: int):
    from PIL import Image

    canvas = Image.new("RGBA", (DISPLAY_W, DISPLAY_H), (0, 0, 0, 0))
    layers = png_layers(build.folder)
    if not layers:
        raise SystemExit(f"No PNG layers found in {build.folder}")

    for layer_path in layers:
        layer = Image.open(layer_path).convert("RGBA")
        if layer.size != (DISPLAY_W, DISPLAY_H):
            raise SystemExit(f"{layer_path} is {layer.size[0]}x{layer.size[1]}. Expected {DISPLAY_W}x{DISPLAY_H}.")

        pixels = layer.load()
        for y in range(DISPLAY_H):
            for x in range(DISPLAY_W):
                r, g, b, a = pixels[x, y]
                if a < alpha_threshold or (r <= black_threshold and g <= black_threshold and b <= black_threshold):
                    pixels[x, y] = (0, 0, 0, 0)

        canvas.alpha_composite(layer)

    canvas.save(build.folder / "emote.png")
    return canvas


def pixels_from_image(image) -> list[tuple[int, int, int]]:
    pixels: list[tuple[int, int, int]] = []
    data = image.load()
    for y in range(DISPLAY_H):
        for x in range(DISPLAY_W):
            r, g, b, a = data[x, y]
            if a > 0:
                pixels.append((x, y, rgb565(r, g, b)))
    return pixels


def escape_cpp_string(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


def write_header(output: Path, emotes: list[tuple[EmoteBuild, list[tuple[int, int, int]]]]) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    lines: list[str] = [
        "#pragma once",
        "#include <Arduino.h>",
        '#include "SpriteAssets.h"',
        "",
        "// Generated by firmware/tools/user_emotes/build_user_emotes.py.",
        "// Edit PNG layers in firmware/user_emotes, then rerun the tool.",
        "",
        "struct UserEmoteAsset {",
        "  const char* name;",
        "  uint16_t weight;",
        "  unsigned long durationMs;",
        "  const SparseAsset* asset;",
        "};",
        "",
    ]

    scaled_weights: list[int] = []
    for index, (build, pixels) in enumerate(emotes):
        symbol = symbol_name(build.name, index)
        weight = max(1, int(round(build.weight * 100.0)))
        scaled_weights.append(weight)
        lines.append(f"static const AssetPixel {symbol}_PIXELS[] PROGMEM = {{")
        if pixels:
            for x, y, color in pixels:
                lines.append(f"  {{{x}, {y}, 0x{color:04X}}},")
        else:
            lines.append("  {0, 0, 0x0000},")
        lines.append("};")
        lines.append(f"static const SparseAsset {symbol} = {{ {DISPLAY_W}, {DISPLAY_H}, {len(pixels)}, {symbol}_PIXELS }};")
        lines.append("")

    count = len(emotes)
    total_weight = sum(scaled_weights)
    lines.append(f"constexpr uint8_t USER_EMOTE_COUNT = {count};")
    lines.append(f"constexpr uint16_t USER_EMOTE_TOTAL_WEIGHT = {total_weight};")
    lines.append("")
    lines.append("static const UserEmoteAsset USER_EMOTES[] = {")
    if emotes:
        for index, (build, _pixels) in enumerate(emotes):
            symbol = symbol_name(build.name, index)
            lines.append(f'  {{"{escape_cpp_string(build.name)}", {scaled_weights[index]}, {build.duration_ms}, &{symbol}}},')
    else:
        lines.append('  {"", 0, 0, nullptr},')
    lines.append("};")
    lines.append("")

    output.write_text("\n".join(lines), encoding="utf-8", newline="\n")


def main() -> None:
    args = parse_args()
    source = args.source.resolve()
    output = args.output.resolve() if args.output else latest_sketch_dir() / "UserEmotes.h"

    print("MP3-Gotchi user emote builder")
    print("=============================")
    print("This tool combines full-screen PNG layers into firmware emotes.")
    print(f"Source folder: {source}")
    print(f"Output header: {output}")

    try:
        import PIL  # noqa: F401
    except ImportError as exc:
        raise SystemExit("Pillow is required. Run: py -m pip install pillow") from exc

    source.mkdir(parents=True, exist_ok=True)
    folders = find_emote_folders(source)
    if not folders:
        readme = source / "README.txt"
        if not readme.exists():
            readme.write_text(
                "Create one folder per emote here.\n"
                "Example: firmware/user_emotes/happy/01-eyes.png, 02-mouth.png, 03-nose.png\n"
                "PNG files must be 96x64. Black and transparent pixels are treated as empty.\n",
                encoding="utf-8",
            )
        write_header(output, [])
        print("\nNo PNG layers were found.")
        print(f"Created {readme}")
        print(f"Wrote empty {output}")
        return

    selected = choose_folders(folders, args)
    if not selected:
        print("No emotes selected. Nothing changed.")
        return

    builds = collect_builds(source, selected, args)
    generated: list[tuple[EmoteBuild, list[tuple[int, int, int]]]] = []
    for build in builds:
        image = compose_emote(build, args.black_threshold, args.alpha_threshold)
        pixels = pixels_from_image(image)
        generated.append((build, pixels))
        print(f"Built {build.name}: {len(pixels)} pixels, preview {build.folder / 'emote.png'}")

    write_header(output, generated)
    print(f"\nWrote {output}")
    print("Done. Upload the latest firmware sketch to use these emotes.")


if __name__ == "__main__":
    main()
