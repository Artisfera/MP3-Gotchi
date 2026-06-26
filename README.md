# MP3-Gotchi

MP3-Gotchi is a tiny Tamagotchi-style offline MP3 player with a small monkey companion on a 96x64 color OLED.

The project is built around an ESP32, SSD1331 OLED, rotary encoder, YX5300 MP3 module, MPU6050 motion sensor, and haptic motor. It is shared as a community-source, source-available, non-commercial hardware and firmware project.

Current firmware release: `mp3gotchi-0.2.0`.

## Quick Start

Open the latest Arduino sketch here:

```text
firmware/src/mp3gotchi-0.2.0/mp3gotchi-0.2.0.ino
```

User-facing firmware settings live in:

```text
firmware/src/mp3gotchi-0.2.0/UserConfig.h
```

Start there for messages, track count, YX5300 track offset, volume, brightness, encoder steps, shake thresholds, and custom emote timing.

## Repository Layout

```text
firmware/assets/          Source sprite assets and sprite reference files.
firmware/src/             Latest Arduino sketch only.
firmware/reversions/      Historical firmware snapshots.
firmware/tools/           Community helper scripts for SD cards, fonts, and emotes.
firmware/user_emotes/     User workspace for custom emote layers.
hardware/3dmodels/        Enclosure exports by version.
hardware/drivers/         Driver download notes.
hardware/schematics/      Prototype schematic exports.
timelapses/               Prototype build videos and timelapse documentation.
```

## Firmware

The latest firmware uses:

- YX5300 UART playback,
- SSD1331 color OLED rendering,
- rotary encoder click and rotation controls,
- MPU6050 shake detection,
- haptic feedback,
- persistent ESP32 Preferences for track, volume, brightness, and shake settings,
- generated sparse RGB565 custom emotes,
- generated Aseprite Mini bitmap font data.

More firmware details are in `firmware/README.md` and the latest sketch README.

## Community Helpers

Prepare a YX5300 SD card in deterministic alphabetical copy order:

```powershell
python .\firmware\tools\prepare_yx5300_sd.py
```

Build custom emotes from full-screen PNG layers:

```powershell
py .\firmware\tools\user_emotes\build_user_emotes.py
```

Regenerate the Aseprite Mini font header from a local `.otf` file:

```powershell
python .\firmware\tools\generate_aseprite_mini_font.py --input C:\path\to\aseprite-mini.otf --output .\firmware\src\mp3gotchi-0.2.0\AsepriteMiniFontData.h --font-size 5
```

Local songs belong in `firmware/tools/songs/` while preparing an SD card. Real audio files are intentionally ignored by Git.

## Local Folders

`.git` is required by Git. It contains repository history, branches, and commit metadata. Do not delete it if you want commits to work.

`.venv` is a local Python virtual environment for helper scripts. It is useful on your machine but should not be committed.

`.backup` is local backup storage created during firmware work. It is ignored by Git.

## License and Use

MP3-Gotchi is shared for personal builds, learning, repair, modification, forks, and non-commercial community collaboration.

Commercial use is not allowed without written permission from Patryk Ankudowicz (Artisfera).

Because the project keeps a non-commercial restriction, it is not an OSI-approved open-source license. In public wording, prefer "community-source", "source-available", or "community-friendly non-commercial".

Firmware, helper scripts, and source code use the PolyForm Noncommercial License 1.0.0. Documentation, graphics, hardware files, models, schematics, videos, and media use CC BY-NC-SA 4.0 unless a file says otherwise. Third-party material is listed in `NOTICE`.

See `LICENSE` and `NON_COMMERCIAL_NOTICE.md`.

## AI Disclosure

Firmware and repository cleanup were prepared in collaboration with ChatGPT as a programming assistant.

The project concept, hardware direction, creative decisions, and authorship belong to Patryk Ankudowicz (Artisfera).
