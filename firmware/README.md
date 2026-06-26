# Firmware

The latest Arduino sketch is kept in `firmware/src/`.

For the current release, open:

```text
firmware/src/mp3gotchi-0.2.0/mp3gotchi-0.2.0.ino
```

Historical snapshots live in `firmware/reversions/`.

## Editing

Start with:

```text
firmware/src/mp3gotchi-0.2.0/UserConfig.h
```

This file is meant for community editing. It contains user-facing settings such as messages, default volume, brightness, track count, YX5300 index offset, encoder behavior, text scroll timing, shake thresholds, and custom emote timing.

Internal pin and firmware constants are kept in `ProjectConfig.h` and `PinMap.h`.

## Tools

- `tools/prepare_yx5300_sd.py` copies local songs to an SD card in deterministic alphabetical order.
- `tools/user_emotes/build_user_emotes.py` converts full-screen PNG layers into sparse RGB565 firmware assets.
- `tools/generate_aseprite_mini_font.py` generates the Aseprite Mini bitmap font header from a local `.otf` file.

Put local audio files in `tools/songs/` while preparing an SD card. The helper uses this folder by default no matter where it is launched from. The folder is ignored by Git except for its README.

## Arduino Libraries

Install these with Arduino IDE Library Manager:

- Adafruit SSD1331 OLED Driver Library
- Adafruit GFX Library

The MPU6050 is read through `Wire`.
The YX5300 module is controlled directly through UART frames.

## Versioning

`firmware/src/` contains the latest working sketch only.

`firmware/reversions/` keeps historical snapshots named like:

```text
mp3gotchi-0.1.13
mp3gotchi-0.2.0
```

The Arduino sketch folder name and `.ino` filename must match.

## License and Use

Firmware in this folder is part of MP3-Gotchi and follows the repository `LICENSE`.

It is community-source and source-available for personal building, learning, repair, modification, and non-commercial community collaboration.

Commercial use is not allowed without written permission from Patryk Ankudowicz (Artisfera).

Because the project keeps a non-commercial restriction, it is not OSI-approved open source. Prefer "community-source", "source-available", or "community-friendly non-commercial" in public wording.
