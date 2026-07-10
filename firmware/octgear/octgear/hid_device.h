#pragma once

#include <Arduino.h>
#include "config.h"

void beginHidDevice();
void updateHidDevice();
bool hidDeviceMounted();
bool remapperConnected();
void sendKeyChanges(Config::KeyMask oldMask, Config::KeyMask newMask, uint8_t layer);
