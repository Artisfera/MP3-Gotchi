#pragma once
#include <Arduino.h>

namespace Pins {
  constexpr uint8_t OLED_SCK  = 18;
  constexpr uint8_t OLED_MOSI = 23;
  constexpr uint8_t OLED_RST  = 14;
  constexpr uint8_t OLED_DC   = 13;
  constexpr uint8_t OLED_CS   = 4;

  constexpr uint8_t YX_RX_FROM_MODULE = 17; // ESP32 RX, connected to YX5300 TX.
  constexpr uint8_t YX_TX_TO_MODULE   = 16; // ESP32 TX, connected to YX5300 RX through 1k.

  constexpr uint8_t ENCODER_A  = 27;
  constexpr uint8_t ENCODER_B  = 22;
  constexpr uint8_t ENCODER_SW = 34; // External 10k pull-up required. GPIO34 has no internal pull-up.

  constexpr uint8_t MOTOR_GATE = 12;

  constexpr uint8_t MPU_SCL = 5;
  constexpr uint8_t MPU_SDA = 15;
  constexpr uint8_t MPU_INT = 39;

  constexpr uint8_t FUNCTION_1_RESERVED = 33;
  constexpr uint8_t FUNCTION_2_RESERVED = 32;
}
