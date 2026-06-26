# MP3-Gotchi firmware v0.1.4

This is the MP3-Gotchi 0.1.4 raw progress firmware. It keeps the tested 0.1.3 behavior, leaves the working YX5300 pinout unchanged, and replaces the MPU6050 library path with a direct I2C driver that prints exact init stages.

## Features

- keeps `monkey-bg` as the static body/background layer,
- overlays normal and dizzy face sprites,
- pauses and resumes music with one encoder click,
- changes to the next track after the configured number of right encoder detents,
- restarts the current track on the first left action,
- moves to the previous track when the left action is repeated within the configured time window,
- waits for the full multi-click window so 10 clicks reset track 1 and do not trigger 5-click sleep,
- enters ESP32 deep sleep after five clicks,
- remembers the last estimated track before deep sleep,
- resumes the saved track after wake,
- resets and saves track 1 after ten clicks,
- uses raw MPU6050 I2C reads for shake detection,
- sends YX5300 loop-all fallback on boot and still logs finish packets if they arrive,
- scrolls the bottom message strip from left to right.

## Arduino Libraries

Install these with Arduino IDE Library Manager:

- Adafruit SSD1331 OLED Driver Library
- Adafruit GFX Library

The MPU6050 is read directly through `Wire`, so this version does not need the Adafruit MPU6050 or Adafruit Unified Sensor libraries.
The YX5300 module is controlled directly through UART frames, so it does not need an external audio library.

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
- Function 1 and Function 2 are reserved for future firmware and are not used in this version.

## MPU6050 Debug

On boot the firmware tries both pin orders and both common MPU addresses:

- SDA GPIO5 / SCL GPIO15, addresses `0x68` and `0x69`
- SDA GPIO15 / SCL GPIO5, addresses `0x68` and `0x69`

For each attempt it reads `WHO_AM_I`, accepts `0x68`, `0x70`, and `0x71`, writes `PWR_MGMT_1 = 0x00`, and then reads raw accel/gyro bytes from `0x3B`.
Serial output names the exact failed stage: `address`, `WHO_AM_I`, `wake`, or `read sample`.

## Temporary Serial Commands

These commands are temporary and should be removed after the hardware pinout and YX behavior are confirmed:

```text
pinout
odwroc mpu
odwroc yx
odwroc mpu/yx
loop all on
loop all off
```

`odwroc mpu` swaps SDA/SCL between `5/15` and `15/5`, then reruns MPU init.
`odwroc yx` swaps RX/TX between `16/17` and `17/16`, then sends volume plus play track 1.
`loop all on` and `loop all off` send the YX5300 repeat-all fallback command.

## Track Order

The ESP32 cannot alphabetically sort songs in this hardware layout because it talks only to the YX5300 over UART. It cannot see SD card filenames or directory entries.
Firmware therefore uses global YX5300 track indexes.

For alphabetical playback without requiring `001`, `002`, `003` names, prepare an empty SD card from a sorted source folder:

```powershell
.\firmware\tools\prepare_yx5300_sd.ps1 -Source C:\Music\MP3Gotchi -Destination E:\
```

If your module or card still behaves unpredictably, use the stronger optional copy mode:

```powershell
.\firmware\tools\prepare_yx5300_sd.ps1 -Source C:\Music\MP3Gotchi -Destination E:\ -PrefixNumeric
```

The helper copies files one by one in alphabetical source order. Most YX5300-style modules play according to FAT directory order, so the SD card should be empty before copying.

## Editing Bottom Messages

Edit the block in `MessagePool.cpp`:

```cpp
static const char MESSAGE_SCRIPT[] = R"MSG(
"Milego dnia!", 1.0;
"Dobry vibe", 0.8;
"Ziuu dalej", 0.7;
"Monkey mode", 1.2;
"Enjoy!";
"Shake me!", 0.5;
)MSG";
```

Format:

```cpp
"Text", 0.5;
"Text2";
"Text3", 1.2;
```

When no number is provided, the firmware uses weight `1.0`.
The number is a selection weight, not a percent. For example, `1.2` is more likely than `0.5`.

## Font

The current firmware still contains `TinyFont5` as a temporary fallback so the sketch stays buildable.
It is not claimed to be Aseprite Mini.

To use the real Aseprite Mini font, supply your own licensed PNG sheet and generate a header named `AsepriteMiniFontData.h`:

```powershell
python .\firmware\tools\generate_aseprite_mini_font.py --input C:\Fonts\aseprite-mini.png --output .\firmware\src\mp3gotchi-0.1.4\AsepriteMiniFontData.h
```

After the generated header is available, replace the fallback renderer with the generated glyph table.

## Power Note

Five clicks do not physically disconnect the battery.
This firmware puts the ESP32 into deep sleep, tells the YX5300 to pause/sleep, blanks the OLED, and turns the motor output off.
A real hard power cut would need extra hardware such as a load switch, MOSFET latch, or physical switch.

## AI Disclosure

The `.ino` header intentionally says that the firmware was prepared in collaboration with ChatGPT as a programming assistant.
That note is part of the project process: the device concept, hardware decisions, and creative direction remain with the project author.
