# MP3-Gotchi

A tiny Tamagotchi-style MP3 player built as a personal anniversary gift, and project on Hackaday Europe 2026.

Have you ever been on a trip, lost signal, and ended up sitting in silence because streaming is gone and phone MP3 playback just does not feel the same? My girlfriend has that problem often, so I wanted to build her a small dedicated music player with a cute monkey pet living inside it.

While listening to music, she will be able to feed and take care of the little monkey, unlock small interactions, and treat the player more like a pocket companion than a normal MP3 device.

This repository is the main project log. It includes the idea, hardware plan, BOM, build log, design decisions, and future updates.

## Core idea

MP3-Gotchi combines three things:

1. A real offline MP3 player.
2. A Tamagotchi-style pet interface.
3. A small handmade electronic gadget with a custom enclosure.

The prototype is intentionally built from modules, but the goal is still to keep the design technically honest. The current version uses the built-in ESP32 DAC and a direct microSD connection instead of delegating playback to a black-box MP3 module. The sound will be tested with real wired earphones, because I need a consistent reference pair instead of judging the audio only through Bluetooth or old noisy earbuds.

To make sure she never loses it, the final concept also includes an AirTag-style locator idea. The current plan is to explore an OpenHaystack-like BLE tracking approach or another small tracker solution inside the enclosure.

## Hardware BOM

Main order from msalamon was about $32.10 including InPost parcel locker shipping. Prices below are converted from PLN to USD using an approximate rate of 1 USD = 3.60 PLN, so small rounding differences are expected.

| Component | Purpose | Quantity | Cost | Link |
| --- | --- | ---: | ---: | --- |
| LoLin ESP32 Wemos Lite | Main MCU board. Chosen because it is compact, has ESP32 DAC output, USB-C, and TP4054 LiPo charging onboard. | 1 | $6.56 | [Link](https://sklep.msalamon.pl/produkt/esp32-wemos-lite-4mb-flash-wifi-ble-4-2-li-ion-usb-c/) |
| SSD1331 OLED Display | 0.95 inch 96x64 color SPI OLED for the Tamagotchi-style UI. | 1 | $11.32 | [Link](https://sklep.msalamon.pl/produkt/wyswietlacz-oled-095-spi-kolorowy/) |
| Rotary encoder | 15 mm encoder with push button, used for volume and menu control. Planned with a custom 3D printed clack knob feel. | 1 | $1.39 | [Link](https://sklep.msalamon.pl/produkt/enkoder-obrotowy-15-mm/) |
| MicroSD reader | SPI microSD reader for direct file access from ESP32. | 1 | $1.92 | [Link](https://sklep.msalamon.pl/produkt/modul-czytnika-kart-micro-sd/) |
| MPU6050 accelerometer and gyroscope | Motion sensor for possible shake, tilt, gesture, or pet interaction features. | 1 | $3.61 | [Link](https://sklep.msalamon.pl/produkt/mpu6050-modul/) |
| Mini vibration motor 12000 RPM | Haptic feedback for UI actions, pet reactions, alerts, or game-like interaction. | 2 | $3.72 | [Link](https://sklep.msalamon.pl/produkt/mini-silnik-wibracyjny-12000-rpm/) |
| LiPo battery | Reused cell from a disposable vape. Used as the first prototype battery. | 1 | $0.00 |  |
| Wired earphones | Reference earphones for testing the ESP32 DAC audio, output coupling, noise, and real listening quality. Included because I do not have a better wired reference pair besides Bluetooth audio and old noisy earbuds. | 1 | $7.28 | [Link](https://pl.aliexpress.com/item/1005006510076171.html) |
| InPost shipping | Shipping cost for the main msalamon hardware order. | 1 | $3.59 |  |
| **Total** | **Estimated full prototype cost, including the main hardware order, shipping, reused battery, and wired reference earphones.** |  | **$39.39** |  |

## Design decisions

### Why Wemos Lite?

The prototype uses a LoLin ESP32 Wemos Lite because it gives a compact ESP32 board with USB-C and onboard TP4054 charging. That helps a lot in a small hand-wired build, where adding a separate charger board would cost space and make the wiring messier.

### Why direct ESP32 audio?

The easy route would be to use a DFPlayer-style module, because it handles microSD, MP3 decoding, playback, and audio output as one black-box part.

I decided to remove it from the current BOM and use the ESP32 path instead. It is harder, but also more interesting and gives the main controller more ownership over the device. With direct microSD access, the ESP32 can control the file system and the player logic directly instead of only sending simple commands like play track 001 to an external module.

The risk is audio quality. The ESP32 DAC is not a hi-fi audio codec, so the prototype needs real listening tests with wired earphones. If the audio is clearly unacceptable, the future version may move to a better audio solution, but the current build intentionally starts with the more ambitious direct approach.

### Why a modular prototype?

A proper final version should use a custom PCB. This build does not, because the deadline is too short. The modular version is faster to debug, easier to repair, and more realistic for a working event prototype.

The goal is not to pretend this is production hardware. The goal is to build a working proof of concept that can later be redesigned into a cleaner PCB.