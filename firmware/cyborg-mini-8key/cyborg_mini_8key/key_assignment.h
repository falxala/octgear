#pragma once

#include <Arduino.h>
#include "config.h"

enum class AssignmentKind : uint8_t {
  None = 0,
  Keyboard = 1,
  Consumer = 2,
  LayerCycle = 3,
  MomentaryLayer = 4,
};

struct KeyAssignment {
  AssignmentKind kind;
  uint8_t modifier;
  uint8_t keycodes[Config::KEYBOARD_REPORT_SLOTS];
  uint16_t consumerUsage;
  uint8_t targetLayer;
};

KeyAssignment blankAssignment();
KeyAssignment keyboardAssignment(uint8_t keycode, uint8_t modifier = 0);
KeyAssignment consumerAssignment(uint16_t usage);
KeyAssignment layerCycleAssignment();
KeyAssignment momentaryLayerAssignment(uint8_t layer);
