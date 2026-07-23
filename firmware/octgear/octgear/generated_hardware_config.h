// Generated from hardware/octgear/profile.json. Do not edit by hand.
#pragma once

#include <Arduino.h>

namespace Config {

constexpr uint8_t PHYSICAL_KEY_COUNT = 8;
constexpr uint8_t ENCODER_CONTROL_COUNT = 3;
constexpr uint8_t KEY_COUNT = PHYSICAL_KEY_COUNT + ENCODER_CONTROL_COUNT;
constexpr uint8_t LAYER_COUNT = 8;
constexpr uint8_t DEFAULT_ENABLED_LAYER_MASK = 0x03;
constexpr uint8_t DEFAULT_LAYER_COLORS[LAYER_COUNT][3] = {
  { 255, 255, 255 },
  { 255, 0, 0 },
  { 0, 180, 255 },
  { 0, 255, 80 },
  { 255, 160, 0 },
  { 180, 0, 255 },
  { 0, 255, 220 },
  { 255, 70, 140 },
};
constexpr uint8_t EXTERNAL_RGB_LED_PIN = 14;
constexpr uint8_t MATRIX_ROW_COUNT = 2;
constexpr uint8_t MATRIX_COLUMN_COUNT = 4;

using KeyMask = uint16_t;

constexpr uint8_t MATRIX_ROW_PINS[MATRIX_ROW_COUNT] = {
  0, 9
};

constexpr uint8_t MATRIX_COLUMN_PINS[MATRIX_COLUMN_COUNT] = {
  5, 6, 7, 8
};

constexpr uint8_t KEY_MATRIX_ROWS[PHYSICAL_KEY_COUNT] = {
  0, 0, 0, 0, 1, 1, 1, 1
};

constexpr uint8_t KEY_MATRIX_COLUMNS[PHYSICAL_KEY_COUNT] = {
  0, 1, 2, 3, 0, 1, 2, 3
};

constexpr uint8_t ENCODER_A_PIN = 11;
constexpr uint8_t ENCODER_COMMON_PIN = 12;
constexpr uint8_t ENCODER_B_PIN = 13;
constexpr uint8_t ENCODER_SWITCH_PIN = 10;
constexpr uint8_t ENCODER_CCW_KEY_INDEX = 8;
constexpr uint8_t ENCODER_CW_KEY_INDEX = 9;
constexpr uint8_t ENCODER_SWITCH_KEY_INDEX = 10;
constexpr int8_t ENCODER_STEPS_PER_DETENT = 4;
constexpr bool ENCODER_REVERSED = false;

}  // namespace Config
