#pragma once
#include <Arduino.h>

// MP3-Gotchi user-editable settings.
// Start here when customizing the community firmware.

namespace UserConfig {
  constexpr uint8_t DEFAULT_VOLUME = 18;       // YX5300 volume range: 0..30.
  constexpr uint8_t DEFAULT_BRIGHTNESS = 100;  // OLED brightness percent: 0..100.
  constexpr uint16_t USER_TRACK_COUNT = 0;     // Reserved. YX5300 next/previous commands handle normal navigation.

  constexpr int ENCODER_TRANSITIONS_PER_DETENT = 4;
  constexpr int TRACK_CHANGE_DETENTS = 5;
  constexpr int VOLUME_CHANGE_DETENTS = 1;
  constexpr unsigned long PREVIOUS_SECOND_TURN_WINDOW_MS = 2500;

  constexpr uint8_t MESSAGE_APPEAR_CHANCE_PERCENT = 65;
  constexpr unsigned long MESSAGE_SHOW_MS = 4200;
  constexpr uint8_t MAX_MESSAGES = 20;
  constexpr unsigned long TEXT_SCROLL_FRAME_MS = 80;
  constexpr int16_t TEXT_SCROLL_STEP_PX = 1;

  static const char MESSAGE_SCRIPT[] = R"MSG(
"Milego dnia!", 1.0;
"Dobry vibe", 0.8;
"Ziuu dalej", 0.7;
"Monkey mode", 1.2;
"Enjoy!";
"Shake me!", 0.5;
)MSG";
}
