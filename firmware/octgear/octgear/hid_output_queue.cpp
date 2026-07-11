#include "hid_output_queue.h"

#include "config.h"
#include "hid_reports.h"

namespace {

struct KeyboardHoldState {
  uint8_t modifier;
  uint8_t keycodes[Config::KEYBOARD_REPORT_SLOTS];
  bool active;
};

struct ConsumerHoldState {
  uint16_t usage;
  uint32_t order;
  bool active;
};

constexpr uint8_t HID_ERROR_ROLLOVER = 0x01;
constexpr uint8_t CONSUMER_TAP_QUEUE_SIZE = 8;
constexpr uint32_t CONSUMER_TAP_DURATION_US = 2000;

KeyboardHoldState keyboardHolds[Config::KEY_COUNT] = {};
uint8_t keyboardReportModifier = 0;
uint8_t keyboardReportKeycodes[Config::KEYBOARD_REPORT_SLOTS] = {};
bool keyboardReportDirty = false;

ConsumerHoldState consumerHolds[Config::KEY_COUNT] = {};
uint32_t nextConsumerHoldOrder = 1;
uint16_t consumerReportValue = 0;
uint16_t lastConsumerReportValue = 0;
bool consumerReportDirty = false;
bool pendingConsumerReportIsTap = false;
bool consumerTapActive = false;
uint32_t consumerTapReleaseDueUs = 0;
uint16_t consumerTapQueue[CONSUMER_TAP_QUEUE_SIZE] = {};
uint8_t consumerTapQueueHead = 0;
uint8_t consumerTapQueueCount = 0;

bool timeReached(uint32_t nowUs, uint32_t deadlineUs) {
  return static_cast<uint32_t>(nowUs - deadlineUs) < 0x80000000UL;
}

bool containsKeycode(const uint8_t* keycodes, uint8_t count, uint8_t keycode) {
  for (uint8_t index = 0; index < count; index++) {
    if (keycodes[index] == keycode) {
      return true;
    }
  }
  return false;
}

bool keyboardReportChanged(uint8_t modifier, const uint8_t* keycodes) {
  if (keyboardReportModifier != modifier) {
    return true;
  }

  for (uint8_t slot = 0; slot < Config::KEYBOARD_REPORT_SLOTS; slot++) {
    if (keyboardReportKeycodes[slot] != keycodes[slot]) {
      return true;
    }
  }
  return false;
}

void rebuildKeyboardReport() {
  uint8_t modifier = 0;
  uint8_t keycodes[Config::KEYBOARD_REPORT_SLOTS] = {};
  uint8_t keycodeCount = 0;
  bool rollover = false;

  for (uint8_t keyIndex = 0; keyIndex < Config::KEY_COUNT; keyIndex++) {
    const KeyboardHoldState& hold = keyboardHolds[keyIndex];
    if (!hold.active) {
      continue;
    }

    modifier |= hold.modifier;
    for (uint8_t slot = 0; slot < Config::KEYBOARD_REPORT_SLOTS; slot++) {
      const uint8_t keycode = hold.keycodes[slot];
      if (keycode == 0 || containsKeycode(keycodes, keycodeCount, keycode)) {
        continue;
      }
      if (keycodeCount >= Config::KEYBOARD_REPORT_SLOTS) {
        rollover = true;
        break;
      }
      keycodes[keycodeCount++] = keycode;
    }

    if (rollover) {
      break;
    }
  }

  if (rollover) {
    for (uint8_t slot = 0; slot < Config::KEYBOARD_REPORT_SLOTS; slot++) {
      keycodes[slot] = HID_ERROR_ROLLOVER;
    }
  }

  if (!keyboardReportChanged(modifier, keycodes)) {
    return;
  }

  keyboardReportModifier = modifier;
  for (uint8_t slot = 0; slot < Config::KEYBOARD_REPORT_SLOTS; slot++) {
    keyboardReportKeycodes[slot] = keycodes[slot];
  }
  keyboardReportDirty = true;
}

uint16_t latestHeldConsumerUsage() {
  uint16_t usage = 0;
  uint32_t latestOrder = 0;
  for (uint8_t keyIndex = 0; keyIndex < Config::KEY_COUNT; keyIndex++) {
    const ConsumerHoldState& hold = consumerHolds[keyIndex];
    if (hold.active && hold.order >= latestOrder) {
      usage = hold.usage;
      latestOrder = hold.order;
    }
  }
  return usage;
}

bool enqueueConsumerTap(uint16_t usage) {
  if (usage == 0 || consumerTapQueueCount >= CONSUMER_TAP_QUEUE_SIZE) {
    return false;
  }

  const uint8_t index = static_cast<uint8_t>((consumerTapQueueHead + consumerTapQueueCount) % CONSUMER_TAP_QUEUE_SIZE);
  consumerTapQueue[index] = usage;
  consumerTapQueueCount++;
  return true;
}

uint16_t dequeueConsumerTap() {
  const uint16_t usage = consumerTapQueue[consumerTapQueueHead];
  consumerTapQueueHead = static_cast<uint8_t>((consumerTapQueueHead + 1) % CONSUMER_TAP_QUEUE_SIZE);
  consumerTapQueueCount--;
  return usage;
}

void queueConsumerReport(uint16_t value, bool isTap = false) {
  consumerReportValue = value;
  pendingConsumerReportIsTap = isTap;
  consumerReportDirty = true;
}

}  // namespace

void queueKeyboardAssignment(uint8_t keyIndex, const KeyAssignment& assignment) {
  if (keyIndex >= Config::KEY_COUNT) {
    return;
  }

  KeyboardHoldState& hold = keyboardHolds[keyIndex];
  hold.modifier = assignment.modifier;
  for (uint8_t slot = 0; slot < Config::KEYBOARD_REPORT_SLOTS; slot++) {
    hold.keycodes[slot] = assignment.keycodes[slot];
  }
  hold.active = true;
  rebuildKeyboardReport();
}

void queueKeyboardRelease(uint8_t keyIndex) {
  if (keyIndex >= Config::KEY_COUNT || !keyboardHolds[keyIndex].active) {
    return;
  }

  keyboardHolds[keyIndex].active = false;
  rebuildKeyboardReport();
}

void queueAllKeyboardReleases() {
  for (uint8_t keyIndex = 0; keyIndex < Config::KEY_COUNT; keyIndex++) {
    keyboardHolds[keyIndex].active = false;
  }
  rebuildKeyboardReport();
}

void queueConsumerPress(uint8_t keyIndex, uint16_t usage) {
  if (keyIndex >= Config::KEY_COUNT || usage == 0) {
    return;
  }

  consumerHolds[keyIndex] = { usage, nextConsumerHoldOrder++, true };
}

void queueConsumerRelease(uint8_t keyIndex) {
  if (keyIndex < Config::KEY_COUNT) {
    consumerHolds[keyIndex].active = false;
  }
}

void queueAllConsumerReleases() {
  for (uint8_t keyIndex = 0; keyIndex < Config::KEY_COUNT; keyIndex++) {
    consumerHolds[keyIndex].active = false;
  }

  consumerTapQueueHead = 0;
  consumerTapQueueCount = 0;
  consumerTapActive = false;
  if (consumerReportDirty || lastConsumerReportValue != 0) {
    queueConsumerReport(0);
  }
}

void queueConsumerTap(uint16_t usage) {
  enqueueConsumerTap(usage);
}

void serviceConsumerReports() {
  if (consumerReportDirty) {
    return;
  }

  if (consumerTapActive) {
    if (timeReached(micros(), consumerTapReleaseDueUs)) {
      consumerTapActive = false;
      queueConsumerReport(latestHeldConsumerUsage());
    }
    return;
  }

  if (consumerTapQueueCount > 0) {
    if (lastConsumerReportValue != 0) {
      queueConsumerReport(0);
    } else {
      queueConsumerReport(dequeueConsumerTap(), true);
    }
    return;
  }

  const uint16_t heldUsage = latestHeldConsumerUsage();
  if (heldUsage != lastConsumerReportValue) {
    queueConsumerReport(heldUsage);
  }
}

bool flushKeyboardReport(Adafruit_USBD_HID& usbHid) {
  if (!keyboardReportDirty || !usbHid.ready()) {
    return false;
  }

  if (!usbHid.keyboardReport(RID_KEYBOARD, keyboardReportModifier, keyboardReportKeycodes)) {
    return false;
  }

  keyboardReportDirty = false;
  return true;
}

bool flushConsumerReport(Adafruit_USBD_HID& usbHid) {
  if (!consumerReportDirty || !usbHid.ready()) {
    return false;
  }

  if (!usbHid.sendReport16(RID_CONSUMER_CONTROL, consumerReportValue)) {
    return false;
  }

  lastConsumerReportValue = consumerReportValue;
  consumerReportDirty = false;
  if (pendingConsumerReportIsTap) {
    consumerTapActive = true;
    consumerTapReleaseDueUs = micros() + CONSUMER_TAP_DURATION_US;
  }
  pendingConsumerReportIsTap = false;
  return true;
}
