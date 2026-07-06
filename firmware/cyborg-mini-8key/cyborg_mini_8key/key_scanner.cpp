#include "key_scanner.h"

#include "config.h"

#if defined(ARDUINO_ARCH_RP2040)
#include "hardware/gpio.h"
#endif

#ifndef digitalReadFast
#define digitalReadFast digitalRead
#endif

namespace {

uint8_t stableMask = 0;
uint8_t lastRawMask = 0;
uint8_t oldStableMask = 0;
uint32_t lastRawChangeUs = 0;

#if defined(ARDUINO_ARCH_RP2040)
static_assert(Config::KEY_COUNT == 8);
static_assert(Config::KEY_PINS[0] == 7);
static_assert(Config::KEY_PINS[1] == 6);
static_assert(Config::KEY_PINS[2] == 5);
static_assert(Config::KEY_PINS[3] == 4);
static_assert(Config::KEY_PINS[4] == 12);
static_assert(Config::KEY_PINS[5] == 11);
static_assert(Config::KEY_PINS[6] == 10);
static_assert(Config::KEY_PINS[7] == 9);

uint8_t readRawKeyMaskFast() {
  const uint32_t pins = ~gpio_get_all();
  uint8_t mask = 0;

  mask |= static_cast<uint8_t>(((pins >> 7) & 1U) << 0);
  mask |= static_cast<uint8_t>(((pins >> 6) & 1U) << 1);
  mask |= static_cast<uint8_t>(((pins >> 5) & 1U) << 2);
  mask |= static_cast<uint8_t>(((pins >> 4) & 1U) << 3);
  mask |= static_cast<uint8_t>(((pins >> 12) & 1U) << 4);
  mask |= static_cast<uint8_t>(((pins >> 11) & 1U) << 5);
  mask |= static_cast<uint8_t>(((pins >> 10) & 1U) << 6);
  mask |= static_cast<uint8_t>(((pins >> 9) & 1U) << 7);

  return mask;
}
#endif

uint8_t readRawKeyMask() {
#if defined(ARDUINO_ARCH_RP2040)
  return readRawKeyMaskFast();
#else
  uint8_t mask = 0;

  for (uint8_t i = 0; i < Config::KEY_COUNT; i++) {
    if (!digitalReadFast(Config::KEY_PINS[i])) {
      mask |= static_cast<uint8_t>(1U << i);
    }
  }

  return mask;
#endif
}

}  // namespace

void beginKeyScanner() {
  for (uint8_t i = 0; i < Config::VIRTUAL_GROUND_COUNT; i++) {
    digitalWrite(Config::VIRTUAL_GROUND_PINS[i], LOW);
    pinMode(Config::VIRTUAL_GROUND_PINS[i], OUTPUT);
  }

  for (uint8_t i = 0; i < Config::KEY_COUNT; i++) {
    pinMode(Config::KEY_PINS[i], INPUT_PULLUP);
  }

  stableMask = readRawKeyMask();
  oldStableMask = stableMask;
  lastRawMask = stableMask;
  lastRawChangeUs = micros();
}

bool updateKeyScanner(bool lowLatencyPress) {
  const uint8_t rawMask = readRawKeyMask();
  const uint32_t nowUs = micros();

  if (rawMask != lastRawMask) {
    lastRawMask = rawMask;
    lastRawChangeUs = nowUs;

    if (lowLatencyPress) {
      const uint8_t newlyPressed = rawMask & ~stableMask;
      if (newlyPressed != 0) {
        oldStableMask = stableMask;
        stableMask |= newlyPressed;
        return true;
      }
    }

    return false;
  }

  if (rawMask == stableMask) {
    return false;
  }

  if (static_cast<uint32_t>(nowUs - lastRawChangeUs) < Config::DEBOUNCE_US) {
    return false;
  }

  oldStableMask = stableMask;
  stableMask = rawMask;
  return true;
}

uint8_t currentKeyMask() {
  return stableMask;
}

uint8_t previousKeyMask() {
  return oldStableMask;
}
