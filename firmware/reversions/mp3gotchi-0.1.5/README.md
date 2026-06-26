# MP3-Gotchi firmware v0.1.5

This is the MP3-Gotchi 0.1.5 raw progress firmware. It keeps the working YX5300 UART pinout, adds stronger MPU6050 diagnostics, fixes end-of-track playback, adds OLED brightness control, and moves user-facing settings into one community-friendly file.

## Edit This First

Customize user settings in `UserConfig.h`.

That file contains:

- default YX5300 volume,
- default OLED brightness,
- known track count for wrap-to-track-1,
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
- YX5300 finish events explicitly start the next track,
- MPU diagnostics try both pin orders, addresses `0x68/0x69`, and I2C speeds `100k/50k/400k`.

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
odwroc mpu
odwroc yx
odwroc mpu/yx
loop all on
loop all off
```

`mpu diag` reruns full MPU detection.
`mpu scan` scans I2C on the active and swapped pin orders.
`mpu pins <sda> <scl>` lets you test a manual pin pair.
`loop all` commands are kept only as temporary YX5300 hardware debug helpers; normal firmware uses explicit next-track play commands.

## Track Order

The ESP32 cannot alphabetically sort songs in this hardware layout because it talks only to the YX5300 over UART. It cannot see SD card filenames or directory entries.
Firmware uses global YX5300 track indexes.

Set `USER_TRACK_COUNT` in `UserConfig.h` if you know how many tracks are on the SD card. When it is `0`, firmware advances upward without wrap-to-1.

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
