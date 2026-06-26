#pragma once
#include <Arduino.h>

// MP3-Gotchi user-editable settings.
// Start here when customizing the community firmware.

namespace UserConfig {
  constexpr uint8_t DEFAULT_VOLUME = 18;       // YX5300 volume range: 0..30.
  constexpr uint8_t DEFAULT_BRIGHTNESS = 100;  // OLED brightness percent: 0..100.
  constexpr uint16_t USER_TRACK_COUNT = 4;     // Set to 0 if the module track count is unknown.
  constexpr uint16_t YX_TRACK_INDEX_OFFSET = 1; // Logical 001 maps to module track 2 on the current hardware.

  constexpr int ENCODER_TRANSITIONS_PER_DETENT = 4;
  constexpr int TRACK_CHANGE_DETENTS = 5;
  constexpr int VOLUME_CHANGE_DETENTS = 1;
  constexpr int BRIGHTNESS_CHANGE_DETENTS = 1;
  constexpr uint8_t BRIGHTNESS_CHANGE_STEP_PERCENT = 5;
  constexpr unsigned long PREVIOUS_SECOND_TURN_WINDOW_MS = 2500;

  constexpr uint8_t MESSAGE_APPEAR_CHANCE_PERCENT = 65;
  constexpr unsigned long MESSAGE_SHOW_MS = 4200;
  constexpr uint8_t MAX_MESSAGES = 20;
  constexpr unsigned long TEXT_SCROLL_FRAME_MS = 80;
  constexpr int16_t TEXT_SCROLL_STEP_PX = 1;

  constexpr bool USER_EMOTES_ENABLED = true;
  constexpr uint8_t USER_EMOTE_APPEAR_CHANCE_PERCENT = 35;
  constexpr unsigned long USER_EMOTE_ROLL_INTERVAL_MS = 60000;
  constexpr unsigned long USER_EMOTE_DEFAULT_SHOW_MS = 2400;

  constexpr bool SHAKE_ENABLED = true;
  constexpr float SHAKE_ACCEL_TOTAL_G_THRESHOLD = 0.85f;   // Required total-g difference from normal gravity.
  constexpr float SHAKE_ACCEL_CHANGE_G_THRESHOLD = 0.50f;  // Required frame-to-frame total-g change.
  constexpr float SHAKE_GYRO_DPS_THRESHOLD = 360.0f;       // Required rotation speed in degrees per second.
  constexpr uint8_t SHAKE_CONFIRM_SAMPLES = 2;             // Consecutive strong samples before a shake is accepted.
  constexpr unsigned long SHAKE_COOLDOWN_MS = 900;         // Delay after a detected shake.

  static const char MESSAGE_SCRIPT[] = R"MSG(
"Milego dnia", 1.0;
"Dobry vibe", 0.8;
"Ziuu dalej", 0.7;
"Monkey mode", 1.2;
"Enjoy";
"Shake me", 0.5;
)MSG";
}
