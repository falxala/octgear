#pragma once

#include <Arduino.h>
#include "config.h"
#include "key_assignment.h"

void beginKeymap();
uint8_t activeLayer();
void setActiveLayer(uint8_t layer);
uint8_t enabledLayerMask();
bool layerEnabled(uint8_t layer);
bool setLayerEnabled(uint8_t layer, bool enabled);
void setEnabledLayerMask(uint8_t mask);
const KeyAssignment& assignmentFor(uint8_t layer, uint8_t keyIndex);
bool setAssignment(uint8_t layer, uint8_t keyIndex, const KeyAssignment& assignment);
void clearKeymap();
