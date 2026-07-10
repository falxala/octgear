#pragma once

#include <Arduino.h>
#include "config.h"

void beginKeyScanner();
bool updateKeyScanner(bool lowLatencyPress);
Config::KeyMask currentKeyMask();
Config::KeyMask previousKeyMask();
