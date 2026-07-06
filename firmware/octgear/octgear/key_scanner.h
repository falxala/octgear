#pragma once

#include <Arduino.h>

void beginKeyScanner();
bool updateKeyScanner(bool lowLatencyPress);
uint8_t currentKeyMask();
uint8_t previousKeyMask();
