#pragma once

// MP3-Gotchi firmware v0.1.4 configuration.
// Project concept and hardware by Patryk Ankudowicz (Artisfera).
// Firmware created in collaboration with ChatGPT as programming assistant.

namespace Config {
  constexpr int SERIAL_BAUD = 115200;

  constexpr bool AUTOPLAY_ON_BOOT = true;
  constexpr uint8_t START_FOLDER = 1;
  constexpr uint16_t START_TRACK = 1;
  constexpr uint8_t DEFAULT_VOLUME = 18;       // YX5300 range is normally 0..30.

  constexpr int ENCODER_TRANSITIONS_PER_DETENT = 4;
  constexpr int TRACK_CHANGE_DETENTS = 5;      // Rotate this many detents to change track.
  constexpr unsigned long PREVIOUS_SECOND_TURN_WINDOW_MS = 2500;
  constexpr uint16_t TRACK_LOOP_LAST_TRACK = 0; // 0 means unknown: use module next/previous commands.
  constexpr bool YX_DEBUG_PACKETS = true;
  constexpr bool YX_ENABLE_LOOP_ALL_FALLBACK = true;

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

  constexpr uint8_t MESSAGE_APPEAR_CHANCE_PERCENT = 65;
  constexpr unsigned long MESSAGE_SHOW_MS = 4200;
  constexpr uint8_t MAX_MESSAGES = 20;
  constexpr unsigned long TEXT_SCROLL_FRAME_MS = 80;
  constexpr int16_t TEXT_SCROLL_STEP_PX = 1;

  constexpr bool DRAW_STATIC_SDCARD_ICON = false;
  constexpr bool DRAW_STATIC_HEADPHONES_ICON = false;
  constexpr bool DRAW_BATTERY_ICON = false; // No battery ADC in current hardware revision.
  constexpr bool DRAW_PLAYING_INDICATOR = false;
}
