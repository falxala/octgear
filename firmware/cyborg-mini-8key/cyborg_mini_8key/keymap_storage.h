#pragma once

#include <Arduino.h>

void beginKeymapStorage();
bool loadKeymapFromStorage();
bool saveKeymapToStorage();
bool saveAssignmentToStorage(uint8_t layer, uint8_t keyIndex);
bool runKeymapStorageSelfTest();
