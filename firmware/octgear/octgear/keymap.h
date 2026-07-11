#pragma once

#include <Arduino.h>
#include "config.h"
#include "key_assignment.h"

struct LayerColor {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

void beginKeymap();
void resetKeymapToDefaults();
uint8_t activeLayer();
void setActiveLayer(uint8_t layer);
uint8_t enabledLayerMask();
bool layerEnabled(uint8_t layer);
bool setLayerEnabled(uint8_t layer, bool enabled);
void setEnabledLayerMask(uint8_t mask);
LayerColor layerColor(uint8_t layer);
bool setLayerColor(uint8_t layer, const LayerColor& color);
void resetLayerColors();
const KeyAssignment& assignmentFor(uint8_t layer, uint8_t keyIndex);
bool setAssignment(uint8_t layer, uint8_t keyIndex, const KeyAssignment& assignment);
void clearKeymap();
