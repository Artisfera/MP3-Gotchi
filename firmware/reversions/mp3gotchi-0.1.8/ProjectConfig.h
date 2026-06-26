#pragma once

// MP3-Gotchi firmware v0.1.8 internal configuration.
// Project concept and hardware by Patryk Ankudowicz (Artisfera).
// Firmware created in collaboration with ChatGPT as programming assistant.

namespace Config {
  constexpr int SERIAL_BAUD = 115200;

  constexpr bool AUTOPLAY_ON_BOOT = true;
  constexpr uint8_t START_FOLDER = 1;
  constexpr uint16_t START_TRACK = 1;
  constexpr bool YX_DEBUG_PACKETS = true;
  constexpr bool YX_ENABLE_LOOP_ALL_FALLBACK = false;

  constexpr unsigned long MULTI_CLICK_GAP_MS = 320;
  constexpr unsigned long DEBOUNCE_MS = 35;
  constexpr unsigned long LONG_PRESS_MS = 850;
  constexpr uint8_t SLEEP_CLICK_COUNT = 5;
  constexpr uint8_t RESET_TRACK_CLICK_COUNT = 10;

  constexpr float SHAKE_ACCEL_DELTA_THRESHOLD = 1.0f;  // m/s^2 above or below gravity.
  constexpr float SHAKE_ACCEL_CHANGE_THRESHOLD = 0.45f; // m/s^2 between sensor samples.
  constexpr float SHAKE_GYRO_THRESHOLD = 0.65f;         // rad/s combined rotation.
  constexpr unsigned long SHAKE_COOLDOWN_MS = 350;
  constexpr unsigned long DIZZY_DURATION_MS = 2200;

  constexpr unsigned long IDLE_FRAME_MS = 650;
  constexpr unsigned long MUSIC_FRAME_MS = 430;
  constexpr unsigned long DIZZY_FRAME_MS = 180;
  constexpr unsigned long BLINK_EVERY_MIN_MS = 3500;
  constexpr unsigned long BLINK_EVERY_MAX_MS = 7200;
  constexpr unsigned long BLINK_DURATION_MS = 140;

  constexpr bool DRAW_STATIC_SDCARD_ICON = false;
  constexpr bool DRAW_STATIC_HEADPHONES_ICON = false;
  constexpr bool DRAW_BATTERY_ICON = false; // No battery ADC in current hardware revision.
  constexpr bool DRAW_PLAYING_INDICATOR = false;
}
