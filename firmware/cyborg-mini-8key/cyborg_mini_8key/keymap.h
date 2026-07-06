#pragma once

#include <Arduino.h>
#include "config.h"
#include "key_assignment.h"

void beginKeymap();
uint8_t activeLayer();
void setActiveLayer(uint8_t layer);
const KeyAssignment& assignmentFor(uint8_t layer, uint8_t keyIndex);
bool setAssignment(uint8_t layer, uint8_t keyIndex, const KeyAssignment& assignment);
void clearKeymap();
