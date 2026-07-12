#include "key_scanner.h"

#include "config.h"
#include "keymap.h"

#ifndef digitalReadFast
#define digitalReadFast digitalRead
#endif

namespace {

Config::KeyMask stableMask = 0;
Config::KeyMask stableInputMask = 0;
Config::KeyMask lastRawInputMask = 0;
Config::KeyMask oldStableMask = 0;
Config::KeyMask activeEncoderPulseMask = 0;
uint32_t lastRawChangeUs = 0;
uint8_t lastEncoderState = 0;
int8_t encoderStepAccumulator = 0;
int8_t pendingEncoderClicks = 0;

constexpr Config::KeyMask keyBit(uint8_t keyIndex) {
  return static_cast<Config::KeyMask>(1U << keyIndex);
}

static_assert(Config::KEY_COUNT <= 16);
static_assert(Config::PHYSICAL_KEY_COUNT == 8);
static_assert(Config::MATRIX_COLUMN_COUNT <= 16);
constexpr Config::KeyMask PHYSICAL_KEY_MASK =
  static_cast<Config::KeyMask>((1U << Config::PHYSICAL_KEY_COUNT) - 1U);

void selectMatrixRow(uint8_t row) {
  const uint8_t pin = Config::MATRIX_ROW_PINS[row];
  digitalWrite(pin, LOW);
  pinMode(pin, OUTPUT);
}

void releaseMatrixRow(uint8_t row) {
  pinMode(Config::MATRIX_ROW_PINS[row], INPUT);
}

Config::KeyMask readRawMatrixMask(bool& ghosted) {
  Config::KeyMask mask = 0;
  uint16_t rowStates[Config::MATRIX_ROW_COUNT] = { 0 };

  for (uint8_t row = 0; row < Config::MATRIX_ROW_COUNT; row++) {
    selectMatrixRow(row);
    delayMicroseconds(1);

    for (uint8_t column = 0; column < Config::MATRIX_COLUMN_COUNT; column++) {
      if (!digitalReadFast(Config::MATRIX_COLUMN_PINS[column])) {
        rowStates[row] |= static_cast<uint16_t>(1U << column);
      }
    }

    releaseMatrixRow(row);
  }

  ghosted = false;
  for (uint8_t first = 0; first < Config::MATRIX_ROW_COUNT; first++) {
    for (uint8_t second = first + 1; second < Config::MATRIX_ROW_COUNT; second++) {
      const uint16_t sharedColumns = rowStates[first] & rowStates[second];
      if ((sharedColumns & static_cast<uint16_t>(sharedColumns - 1U)) != 0) {
        ghosted = true;
      }
    }
  }

  for (uint8_t key = 0; key < Config::PHYSICAL_KEY_COUNT; key++) {
    const uint16_t columnBit = static_cast<uint16_t>(1U << Config::KEY_MATRIX_COLUMNS[key]);
    if ((rowStates[Config::KEY_MATRIX_ROWS[key]] & columnBit) != 0) {
      mask |= keyBit(key);
    }
  }
  return mask;
}

Config::KeyMask readRawInputMask() {
  bool matrixGhosted = false;
  Config::KeyMask mask = readRawMatrixMask(matrixGhosted);
  if (matrixGhosted) {
    mask = stableInputMask & PHYSICAL_KEY_MASK;
  }

  if (!digitalReadFast(Config::ENCODER_SWITCH_PIN)) {
    mask |= keyBit(Config::ENCODER_SWITCH_KEY_INDEX);
  }

  return mask;
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

  const int8_t normalized = encoderReversed() ? static_cast<int8_t>(-direction) : direction;
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
  for (uint8_t row = 0; row < Config::MATRIX_ROW_COUNT; row++) {
    releaseMatrixRow(row);
  }

  for (uint8_t column = 0; column < Config::MATRIX_COLUMN_COUNT; column++) {
    pinMode(Config::MATRIX_COLUMN_PINS[column], INPUT_PULLUP);
  }

  digitalWrite(Config::ENCODER_COMMON_PIN, LOW);
  pinMode(Config::ENCODER_COMMON_PIN, OUTPUT);
  pinMode(Config::ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(Config::ENCODER_B_PIN, INPUT_PULLUP);
  pinMode(Config::ENCODER_SWITCH_PIN, INPUT_PULLUP);

  lastEncoderState = readEncoderState();
  stableInputMask = readRawInputMask();
  stableMask = stableInputMask;
  oldStableMask = stableMask;
  lastRawInputMask = stableInputMask;
  lastRawChangeUs = micros();
}

bool updateKeyScanner(bool lowLatencyPress) {
  updateEncoderState();

  if (activeEncoderPulseMask != 0) {
    activeEncoderPulseMask = 0;
    oldStableMask = stableMask;
    stableMask = stableInputMask;
    return true;
  }

  const Config::KeyMask nextEncoderPulseMask = consumeEncoderPulseMask();
  if (nextEncoderPulseMask != 0) {
    activeEncoderPulseMask = nextEncoderPulseMask;
    oldStableMask = stableMask;
    stableMask = stableInputMask | activeEncoderPulseMask;
    return true;
  }

  const Config::KeyMask rawMask = readRawInputMask();
  const uint32_t nowUs = micros();

  if (rawMask != lastRawInputMask) {
    lastRawInputMask = rawMask;
    lastRawChangeUs = nowUs;

    if (lowLatencyPress) {
      const Config::KeyMask newlyPressed = rawMask & ~stableInputMask;
      if (newlyPressed != 0) {
        stableInputMask |= newlyPressed;
        oldStableMask = stableMask;
        stableMask = stableInputMask;
        return true;
      }
    }

    return false;
  }

  if (rawMask == stableInputMask) {
    return false;
  }

  if (static_cast<uint32_t>(nowUs - lastRawChangeUs) < Config::DEBOUNCE_US) {
    return false;
  }

  oldStableMask = stableMask;
  stableInputMask = rawMask;
  stableMask = stableInputMask;
  return true;
}

Config::KeyMask currentKeyMask() {
  return stableMask;
}

Config::KeyMask previousKeyMask() {
  return oldStableMask;
}
