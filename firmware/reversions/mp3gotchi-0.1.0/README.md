# MP3-Gotchi firmware v0.1.0

This is the main firmware source for the MP3-Gotchi 0.1.0 prototype with a YX5300 audio module.
The code is meant to be copied into a project `src` folder and used as a clean base for later screens, tools, or small games.

## Features

- keeps `monkey-bg` as the static body/background layer,
- overlays face sprites for the normal and dizzy states,
- keeps the idle monkey visible when no special state is active,
- shows the dizzy face after a strong MPU6050 shake,
- pauses and resumes music with one encoder click,
- changes to the next track after the configured number of right encoder detents,
- restarts the current track on the first left action,
- moves to the previous track when the left action is repeated within the configured time window,
- enters ESP32 deep sleep after five encoder clicks,
- requires five clicks after a locked deep-sleep wake before the firmware starts playback again,
- can show a random weighted message in the bottom text strip after a track change.

## Arduino Libraries

Install these with Arduino IDE Library Manager:

- Adafruit SSD1331 OLED Driver Library
- Adafruit GFX Library
- Adafruit MPU6050
- Adafruit Unified Sensor

The YX5300 module is controlled directly through UART frames, so it does not need an external library.

## Pin Map

- OLED SCK: GPIO18
- OLED MOSI: GPIO23
- OLED RST: GPIO14
- OLED DC: GPIO13
- OLED CS: GPIO4
- YX5300 TX to ESP32 RX: GPIO17
- ESP32 TX to YX5300 RX through 1k: GPIO16
- Encoder A: GPIO27
- Encoder B: GPIO22
- Encoder SW: GPIO34, external 10k pull-up required
- Motor MOSFET gate: GPIO12
- MPU6050 SCL: GPIO5
- MPU6050 SDA: GPIO15
- MPU6050 INT: GPIO39
- Function 1 and Function 2 are reserved for future firmware and are not used in this version.

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

## Power Note

Five clicks do not physically disconnect the battery.
This firmware puts the ESP32 into deep sleep, tells the YX5300 to pause/sleep, blanks the OLED, and turns the motor output off.
A real hard power cut would need extra hardware such as a load switch, MOSFET latch, or physical switch.

## AI Disclosure

The `.ino` header intentionally says that the firmware was prepared in collaboration with ChatGPT as a programming assistant.
That note is part of the project process: the device concept, hardware decisions, and creative direction remain with the project author.
