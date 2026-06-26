#pragma once

// MP3-Gotchi firmware v0.1.0 configuration.
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

  constexpr unsigned long MULTI_CLICK_GAP_MS = 320;
  constexpr unsigned long DEBOUNCE_MS = 35;
  constexpr unsigned long LONG_PRESS_MS = 850;
  constexpr uint8_t SLEEP_CLICK_COUNT = 5;

  constexpr bool REQUIRE_5_CLICKS_AFTER_WAKE = true;
  constexpr unsigned long WAKE_GUARD_TIMEOUT_MS = 8000;

  constexpr float SHAKE_ACCEL_DELTA_THRESHOLD = 13.0f; // m/s^2 above or below gravity.
  constexpr float SHAKE_GYRO_THRESHOLD = 7.0f;         // rad/s combined rotation.
  constexpr unsigned long SHAKE_COOLDOWN_MS = 1100;
  constexpr unsigned long DIZZY_DURATION_MS = 1800;

  constexpr unsigned long IDLE_FRAME_MS = 650;
  constexpr unsigned long MUSIC_FRAME_MS = 430;
  constexpr unsigned long DIZZY_FRAME_MS = 180;
  constexpr unsigned long BLINK_EVERY_MIN_MS = 3500;
  constexpr unsigned long BLINK_EVERY_MAX_MS = 7200;
  constexpr unsigned long BLINK_DURATION_MS = 140;

  constexpr uint8_t MESSAGE_APPEAR_CHANCE_PERCENT = 65;
  constexpr unsigned long MESSAGE_SHOW_MS = 4200;
  constexpr uint8_t MAX_MESSAGES = 20;

  constexpr bool DRAW_STATIC_SDCARD_ICON = false;
  constexpr bool DRAW_STATIC_HEADPHONES_ICON = false;
  constexpr bool DRAW_BATTERY_ICON = false; // No battery ADC in current hardware revision.
  constexpr bool DRAW_PLAYING_INDICATOR = false;
}
