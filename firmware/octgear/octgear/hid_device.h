#pragma once

#include <Arduino.h>

void beginHidDevice();
void updateHidDevice();
bool hidDeviceMounted();
bool remapperConnected();
void sendKeyChanges(uint8_t oldMask, uint8_t newMask, uint8_t layer);
