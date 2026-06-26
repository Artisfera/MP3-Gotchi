# MP3-Gotchi firmware v0.1.12

This is the MP3-Gotchi 0.1.12 raw progress firmware. It keeps the working YX5300 and MPU6050 pinouts, keeps logical YX5300 track mapping, adds stronger 1-100 shake sensitivity control, uses software OLED brightness scaling, and keeps user-facing settings in one community-friendly file.

## Edit This First

Customize user settings in `UserConfig.h`.

That file contains:

- default YX5300 volume,
- default OLED brightness,
- logical YX5300 track count and module index offset,
- encoder detent thresholds,
- brightness encoder step,
- shake sensitivity,
- bottom text scroll speed,
- user emote timing and appearance chance,
- random bottom message chance,
- editable weighted message script.

## Features

- one click pauses/resumes playback,
- two clicks switches encoder mode between `Track`, `Volume`, and `Brightness`,
- five clicks enters ESP32 deep sleep,
- ten clicks resets playback to track 1,
- one encoder click wakes from deep sleep through GPIO34,
- track mode rotates through songs and keeps left-turn restart/previous behavior,
- volume mode changes YX5300 volume and saves it to ESP32 Preferences,
- brightness mode changes OLED brightness and saves it to ESP32 Preferences,
- Serial command `brightness 0-100` changes and saves OLED brightness,
- Serial command `shake 1-100` changes and saves MPU6050 shake sensitivity,
- Serial command `reboot` restarts the ESP32 firmware,
- duplicated YX5300 track-finished packets are ignored so automatic playback does not skip every second track,
- bottom text scroll updates only changed pixels to avoid visible blinking,
- bottom messages scroll fully from offscreen left to offscreen right,
- bottom text is rendered with generated Aseprite Mini bitmap glyphs,
- custom user emotes can appear once per minute based on `UserConfig.h` chance settings,
- YX5300 finish events send the module `next` command instead of guessing a numeric track,
- MPU diagnostics default to the confirmed `SDA GPIO15 / SCL GPIO5` wiring,
- `mpu diag` prints 10 sensor samples over 10 seconds,
- `yx diag` prints raw module track and logical track mapping status.

## Font

The bottom text renderer uses `AsepriteMiniFontData.h`, generated from the real `aseprite-mini.otf` file supplied by the project author.
To regenerate it after replacing the font asset:

```powershell
.\.venv\Scripts\python.exe .\firmware\tools\generate_aseprite_mini_font.py --input C:\path\to\aseprite-mini.otf --output .\firmware\src\mp3gotchi-0.1.12\AsepriteMiniFontData.h --font-size 5
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
- MPU6050 SDA: GPIO15
- MPU6050 SCL: GPIO5
- MPU6050 INT: GPIO39
- Function 1 and Function 2 are reserved for future firmware.

## Serial Commands

```text
pinout
brightness
brightness 0
brightness 50
brightness 100
shake
shake 35
shake 80
reboot
mpu diag
yx diag
```

`brightness 0-100` changes the rendered pixel brightness and persists it.
`shake 1-100` changes shake sensitivity and persists it. `1` needs a hard shake, `100` reacts to light movement.
`reboot` restarts the ESP32 firmware with `ESP.restart()`.
`mpu diag` reruns MPU detection and prints 10 accelerometer/gyro samples.
`yx diag` asks the module for current and total tracks, then prints raw module index, logical track, configured offset, and volume.

## Track Order

The ESP32 cannot alphabetically sort songs in this hardware layout because it talks only to the YX5300 over UART. It cannot see SD card filenames or directory entries.
Firmware uses YX5300 track commands and treats numeric track indexes as best-effort status only.

Normal next/previous navigation uses YX5300 module commands, because the module knows its own real track list and the ESP32 cannot read SD filenames through UART.
Absolute play commands use logical-to-module mapping from `UserConfig.h`.
With the current defaults, logical track `1` means file `001`, but the module command receives raw track index `2`.
If your module reports a different offset or track count, edit `USER_TRACK_COUNT` and `YX_TRACK_INDEX_OFFSET`.

## User Emotes

Put full-screen `96x64` PNG layers in `firmware/user_emotes/<emote-name>/`.
Use simple alphabetical prefixes when layer order matters:

```text
firmware/user_emotes/happy/
  01-eyes.png
  02-mouth.png
  03-nose.png
```

Run:

```powershell
py .\firmware\tools\user_emotes\build_user_emotes.py
```

The tool lists detected emote folders, asks which ones to build, asks for each emote weight and duration, writes `emote.png` previews, and generates `UserEmotes.h` in the latest sketch folder.
PNG is only the host-side input and preview format. The ESP32 firmware uses generated sparse RGB565 pixel data from `UserEmotes.h`.
Black pixels and transparent pixels are treated as empty space, matching the source sprite workflow.
Global timing and chance are in `UserConfig.h`:

```cpp
USER_EMOTE_APPEAR_CHANCE_PERCENT
USER_EMOTE_ROLL_INTERVAL_MS
USER_EMOTE_DEFAULT_SHOW_MS
```

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
