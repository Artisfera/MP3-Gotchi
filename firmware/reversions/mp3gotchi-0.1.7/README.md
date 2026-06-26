# MP3-Gotchi firmware v0.1.7

This is the MP3-Gotchi 0.1.7 raw progress firmware. It keeps the working YX5300 UART pinout, uses YX5300 module-relative next/previous commands for normal navigation, keeps stronger MPU6050 diagnostics, uses a generated bitmap font from the supplied real Aseprite Mini OTF asset, and keeps user-facing settings in one community-friendly file.

## Edit This First

Customize user settings in `UserConfig.h`.

That file contains:

- default YX5300 volume,
- default OLED brightness,
- reserved track count setting,
- encoder detent thresholds,
- bottom text scroll speed,
- random bottom message chance,
- editable weighted message script.

## Features

- one click pauses/resumes playback,
- two clicks switches encoder mode between `TRACK` and `VOLUME`,
- five clicks enters ESP32 deep sleep,
- ten clicks resets playback to track 1,
- one encoder click wakes from deep sleep through GPIO34,
- track mode rotates through songs and keeps left-turn restart/previous behavior,
- volume mode changes YX5300 volume and saves it to ESP32 Preferences,
- Serial command `brightness 0-100` changes and saves OLED brightness,
- bottom messages scroll fully from offscreen left to offscreen right,
- bottom text is rendered with generated Aseprite Mini bitmap glyphs,
- YX5300 finish events send the module `next` command instead of guessing a numeric track,
- MPU diagnostics try both pin orders, addresses `0x68/0x69`, and I2C speeds `100k/50k/400k`.

## Font

The bottom text renderer uses `AsepriteMiniFontData.h`, generated from the real `aseprite-mini.otf` file supplied by the project author.
To regenerate it after replacing the font asset:

```powershell
.\.venv\Scripts\python.exe .\firmware\tools\generate_aseprite_mini_font.py --input C:\path\to\aseprite-mini.otf --output .\firmware\src\mp3gotchi-0.1.7\AsepriteMiniFontData.h --font-size 5
```

## Arduino Libraries

Install these with Arduino IDE Library Manager:

- Adafruit SSD1331 OLED Driver Library
- Adafruit GFX Library

The MPU6050 is read directly through `Wire`.
The YX5300 module is controlled directly through UART frames.

## Pin Map

- OLED SCK: GPIO18
- OLED MOSI: GPIO23
- OLED RST: GPIO14
- OLED DC: GPIO13
- OLED CS: GPIO4
- YX5300 TX to ESP32 RX: GPIO16
- ESP32 TX to YX5300 RX through 1k: GPIO17
- Encoder A: GPIO27
- Encoder B: GPIO22
- Encoder SW: GPIO34, external 10k pull-up required
- Motor MOSFET gate: GPIO12
- MPU6050 SDA: GPIO5
- MPU6050 SCL: GPIO15
- MPU6050 INT: GPIO39
- Function 1 and Function 2 are reserved for future firmware.

## Serial Commands

```text
pinout
brightness
brightness 0
brightness 50
brightness 100
mpu diag
mpu scan
mpu pins <sda> <scl>
yx query
odwroc mpu
odwroc yx
odwroc mpu/yx
loop all on
loop all off
```

`mpu diag` reruns full MPU detection.
`mpu scan` scans I2C on the active and swapped pin orders.
`mpu pins <sda> <scl>` lets you test a manual pin pair.
`yx query` asks the module for current track and total track count. Some YX5300-compatible boards answer, some do not.
`loop all` commands are kept only as temporary YX5300 hardware debug helpers; normal firmware uses the module `next` command.

## Track Order

The ESP32 cannot alphabetically sort songs in this hardware layout because it talks only to the YX5300 over UART. It cannot see SD card filenames or directory entries.
Firmware uses YX5300 track commands and treats numeric track indexes as best-effort status only.

Normal next/previous navigation uses YX5300 module commands, because the module knows its own real track list and the ESP32 cannot read SD filenames through UART.
If an older firmware saved an invalid track number, `0.1.7` tries to repair it when the module answers the total-track query.

For deterministic SD copy order, put your audio files in `firmware/tools/songs`, then run the friendly helper:

```powershell
python .\firmware\tools\prepare_yx5300_sd.py
```

It lists available drives with their size, asks you to pick the SD card by number, and then copies songs alphabetically.

## Power Note

Five clicks do not physically disconnect the battery.
This firmware puts the ESP32 into deep sleep, tells the YX5300 to pause/sleep, blanks the OLED, and turns the motor output off.
A real hard power cut needs extra hardware such as a load switch, MOSFET latch, or physical switch.

## License and Use

This project is community-friendly and source-available for personal, educational, repair, modification, and non-commercial collaboration.
Commercial use is not allowed without written permission from Patryk Ankudowicz (Artisfera).

See the repository `LICENSE` and `NON_COMMERCIAL_NOTICE.md`.

## AI Disclosure

The `.ino` header intentionally says that the firmware was prepared in collaboration with ChatGPT as a programming assistant.
The device concept, hardware decisions, and creative direction remain with the project author.
