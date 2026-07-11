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
constexpr uint8_t VIRTUAL_GROUND_COUNT = 2;

using KeyMask = uint16_t;

constexpr uint8_t KEY_PINS[PHYSICAL_KEY_COUNT] = {
  7, 6, 5, 4, 12, 11, 10, 9
};

constexpr uint8_t VIRTUAL_GROUND_PINS[VIRTUAL_GROUND_COUNT] = {
  1, 8
};

constexpr uint8_t ENCODER_A_PIN = 26;
constexpr uint8_t ENCODER_B_PIN = 15;
constexpr uint8_t ENCODER_SWITCH_PIN = 14;
constexpr uint8_t ENCODER_CCW_KEY_INDEX = 8;
constexpr uint8_t ENCODER_CW_KEY_INDEX = 9;
constexpr uint8_t ENCODER_SWITCH_KEY_INDEX = 10;
constexpr int8_t ENCODER_STEPS_PER_DETENT = 2;
constexpr bool ENCODER_REVERSED = false;

}  // namespace Config
