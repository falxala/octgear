#include "key_scanner.h"

#include "config.h"

#if defined(ARDUINO_ARCH_RP2040)
#include "hardware/gpio.h"
#endif

#ifndef digitalReadFast
#define digitalReadFast digitalRead
#endif

namespace {

Config::KeyMask stableMask = 0;
Config::KeyMask stableDirectMask = 0;
Config::KeyMask lastRawDirectMask = 0;
Config::KeyMask oldStableMask = 0;
Config::KeyMask activeEncoderPulseMask = 0;
uint32_t lastRawChangeUs = 0;
uint8_t lastEncoderState = 0;
int8_t encoderStepAccumulator = 0;
int8_t pendingEncoderClicks = 0;

constexpr Config::KeyMask keyBit(uint8_t keyIndex) {
  return static_cast<Config::KeyMask>(1U << keyIndex);
}

#if defined(ARDUINO_ARCH_RP2040)
static_assert(Config::KEY_COUNT <= 16);
static_assert(Config::PHYSICAL_KEY_COUNT == 8);

Config::KeyMask readRawDirectMaskFast() {
  const uint32_t pins = ~gpio_get_all();
  Config::KeyMask mask = 0;

  for (uint8_t i = 0; i < Config::PHYSICAL_KEY_COUNT; i++) {
    mask |= static_cast<Config::KeyMask>(((pins >> Config::KEY_PINS[i]) & 1U) << i);
  }
  mask |= static_cast<Config::KeyMask>(((pins >> Config::ENCODER_SWITCH_PIN) & 1U) << Config::ENCODER_SWITCH_KEY_INDEX);

  return mask;
}
#endif

Config::KeyMask readRawDirectMask() {
#if defined(ARDUINO_ARCH_RP2040)
  return readRawDirectMaskFast();
#else
  Config::KeyMask mask = 0;

  for (uint8_t i = 0; i < Config::PHYSICAL_KEY_COUNT; i++) {
    if (!digitalReadFast(Config::KEY_PINS[i])) {
      mask |= keyBit(i);
    }
  }

  if (!digitalReadFast(Config::ENCODER_SWITCH_PIN)) {
    mask |= keyBit(Config::ENCODER_SWITCH_KEY_INDEX);
  }

  return mask;
#endif
}

uint8_t readEncoderState() {
  const uint8_t a = digitalReadFast(Config::ENCODER_A_PIN) ? 1 : 0;
  const uint8_t b = digitalReadFast(Config::ENCODER_B_PIN) ? 1 : 0;
  return static_cast<uint8_t>((a << 1) | b);
}

void enqueueEncoderClick(int8_t direction) {
  if (direction == 0) {
    return;
  }

  const int8_t normalized = Config::ENCODER_REVERSED ? static_cast<int8_t>(-direction) : direction;
  if (normalized > 0 && pendingEncoderClicks < 4) {
    pendingEncoderClicks++;
  } else if (normalized < 0 && pendingEncoderClicks > -4) {
    pendingEncoderClicks--;
  }
}

void updateEncoderState() {
  static constexpr int8_t kTransitionTable[16] = {
    0, -1, 1, 0,
    1, 0, 0, -1,
    -1, 0, 0, 1,
    0, 1, -1, 0,
  };

  const uint8_t state = readEncoderState();
  if (state == lastEncoderState) {
    return;
  }

  const uint8_t transition = static_cast<uint8_t>((lastEncoderState << 2) | state);
  lastEncoderState = state;
  encoderStepAccumulator += kTransitionTable[transition];

  if (encoderStepAccumulator >= Config::ENCODER_STEPS_PER_DETENT) {
    encoderStepAccumulator = 0;
    enqueueEncoderClick(1);
  } else if (encoderStepAccumulator <= -Config::ENCODER_STEPS_PER_DETENT) {
    encoderStepAccumulator = 0;
    enqueueEncoderClick(-1);
  }
}

Config::KeyMask consumeEncoderPulseMask() {
  if (pendingEncoderClicks > 0) {
    pendingEncoderClicks--;
    return keyBit(Config::ENCODER_CW_KEY_INDEX);
  }

  if (pendingEncoderClicks < 0) {
    pendingEncoderClicks++;
    return keyBit(Config::ENCODER_CCW_KEY_INDEX);
  }

  return 0;
}

}  // namespace

void beginKeyScanner() {
  for (uint8_t i = 0; i < Config::VIRTUAL_GROUND_COUNT; i++) {
    digitalWrite(Config::VIRTUAL_GROUND_PINS[i], LOW);
    pinMode(Config::VIRTUAL_GROUND_PINS[i], OUTPUT);
  }

  for (uint8_t i = 0; i < Config::PHYSICAL_KEY_COUNT; i++) {
    pinMode(Config::KEY_PINS[i], INPUT_PULLUP);
  }

  pinMode(Config::ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(Config::ENCODER_B_PIN, INPUT_PULLUP);
  pinMode(Config::ENCODER_SWITCH_PIN, INPUT_PULLUP);

  lastEncoderState = readEncoderState();
  stableDirectMask = readRawDirectMask();
  stableMask = stableDirectMask;
  oldStableMask = stableMask;
  lastRawDirectMask = stableDirectMask;
  lastRawChangeUs = micros();
}

bool updateKeyScanner(bool lowLatencyPress) {
  updateEncoderState();

  if (activeEncoderPulseMask != 0) {
    activeEncoderPulseMask = 0;
    oldStableMask = stableMask;
    stableMask = stableDirectMask;
    return true;
  }

  const Config::KeyMask nextEncoderPulseMask = consumeEncoderPulseMask();
  if (nextEncoderPulseMask != 0) {
    activeEncoderPulseMask = nextEncoderPulseMask;
    oldStableMask = stableMask;
    stableMask = stableDirectMask | activeEncoderPulseMask;
    return true;
  }

  const Config::KeyMask rawMask = readRawDirectMask();
  const uint32_t nowUs = micros();

  if (rawMask != lastRawDirectMask) {
    lastRawDirectMask = rawMask;
    lastRawChangeUs = nowUs;

    if (lowLatencyPress) {
      const Config::KeyMask newlyPressed = rawMask & ~stableDirectMask;
      if (newlyPressed != 0) {
        stableDirectMask |= newlyPressed;
        oldStableMask = stableMask;
        stableMask = stableDirectMask;
        return true;
      }
    }

    return false;
  }

  if (rawMask == stableDirectMask) {
    return false;
  }

  if (static_cast<uint32_t>(nowUs - lastRawChangeUs) < Config::DEBOUNCE_US) {
    return false;
  }

  oldStableMask = stableMask;
  stableDirectMask = rawMask;
  stableMask = stableDirectMask;
  return true;
}

Config::KeyMask currentKeyMask() {
  return stableMask;
}

Config::KeyMask previousKeyMask() {
  return oldStableMask;
}
