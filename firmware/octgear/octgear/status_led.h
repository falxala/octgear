#pragma once

#include <Arduino.h>

void beginStatusLed();
void setStatusLed(bool on);
void updateStatusHeartbeat(bool mounted, bool remapperConnected, bool rescueActive, uint8_t layer);
