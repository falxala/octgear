#include "hid_output_queue.h"

#include "config.h"
#include "hid_reports.h"

namespace {

struct KeyboardReportState {
  uint8_t modifier;
  uint8_t keycodes[Config::KEYBOARD_REPORT_SLOTS];
  bool dirty;
};

// TinyUSB accepts one HID IN report at a time. Keep reports dirty until accepted
// so key-release reports are retried instead of being dropped while not ready.
KeyboardReportState keyboardReports[Config::KEY_COUNT] = {};
uint8_t nextKeyboardFlushIndex = 0;
bool consumerReleasePending = false;
uint32_t consumerReleaseDueUs = 0;
uint16_t consumerReportValue = 0;
bool consumerReportDirty = false;
constexpr uint8_t CONSUMER_TAP_QUEUE_SIZE = 8;
uint16_t consumerTapQueue[CONSUMER_TAP_QUEUE_SIZE] = {};
uint8_t consumerTapQueueHead = 0;
uint8_t consumerTapQueueCount = 0;

uint8_t keyboardReportIdFor(uint8_t keyIndex) {
  return static_cast<uint8_t>(RID_KEYBOARD_1 + keyIndex);
}

bool timeReached(uint32_t nowUs, uint32_t deadlineUs) {
  return static_cast<uint32_t>(nowUs - deadlineUs) < 0x80000000UL;
}

void queueKeyboardReport(uint8_t reportId, uint8_t modifier, const uint8_t* keycodes) {
  if (reportId < RID_KEYBOARD_1 || reportId > RID_KEYBOARD_8) {
    return;
  }

  KeyboardReportState& report = keyboardReports[reportId - RID_KEYBOARD_1];
  report.modifier = modifier;
  for (uint8_t i = 0; i < Config::KEYBOARD_REPORT_SLOTS; i++) {
    report.keycodes[i] = keycodes[i];
  }
  report.dirty = true;
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
  if (consumerTapQueueCount == 0) {
    return 0;
  }

  const uint16_t usage = consumerTapQueue[consumerTapQueueHead];
  consumerTapQueueHead = static_cast<uint8_t>((consumerTapQueueHead + 1) % CONSUMER_TAP_QUEUE_SIZE);
  consumerTapQueueCount--;
  return usage;
}

void queueConsumerReport(uint16_t value) {
  consumerReportValue = value;
  consumerReportDirty = true;
}

}  // namespace

void queueKeyboardAssignment(uint8_t reportId, const KeyAssignment& assignment) {
  queueKeyboardReport(reportId, assignment.modifier, assignment.keycodes);
}

void queueKeyboardRelease(uint8_t reportId) {
  const uint8_t keycodes[Config::KEYBOARD_REPORT_SLOTS] = { 0 };
  queueKeyboardReport(reportId, 0, keycodes);
}

void queueAllKeyboardReleases() {
  for (uint8_t keyIndex = 0; keyIndex < Config::KEY_COUNT; keyIndex++) {
    queueKeyboardRelease(keyboardReportIdFor(keyIndex));
  }
}

void queueConsumerTap(uint16_t usage) {
  enqueueConsumerTap(usage);
}

void serviceConsumerReports() {
  if (consumerReleasePending && !consumerReportDirty && timeReached(micros(), consumerReleaseDueUs)) {
    queueConsumerReport(0);
    return;
  }

  if (!consumerReleasePending && !consumerReportDirty && consumerTapQueueCount > 0) {
    queueConsumerReport(dequeueConsumerTap());
  }
}

bool flushKeyboardReports(Adafruit_USBD_HID& usbHid) {
  if (!usbHid.ready()) {
    return false;
  }

  for (uint8_t keyIndex = 0; keyIndex < Config::KEY_COUNT; keyIndex++) {
    const uint8_t reportIndex = static_cast<uint8_t>((nextKeyboardFlushIndex + keyIndex) % Config::KEY_COUNT);
    KeyboardReportState& report = keyboardReports[reportIndex];
    if (!report.dirty) {
      continue;
    }

    uint8_t keycodes[Config::KEYBOARD_REPORT_SLOTS] = { 0 };
    for (uint8_t i = 0; i < Config::KEYBOARD_REPORT_SLOTS; i++) {
      keycodes[i] = report.keycodes[i];
    }

    if (usbHid.keyboardReport(keyboardReportIdFor(reportIndex), report.modifier, keycodes)) {
      report.dirty = false;
      nextKeyboardFlushIndex = static_cast<uint8_t>((reportIndex + 1) % Config::KEY_COUNT);
    }

    return true;
  }

  return false;
}

bool flushConsumerReport(Adafruit_USBD_HID& usbHid) {
  if (!consumerReportDirty || !usbHid.ready()) {
    return false;
  }

  const uint16_t value = consumerReportValue;
  if (!usbHid.sendReport16(RID_CONSUMER_CONTROL, value)) {
    return false;
  }

  consumerReportDirty = false;
  if (value == 0) {
    consumerReleasePending = false;
  } else {
    consumerReleaseDueUs = micros() + 2000;
    consumerReleasePending = true;
  }

  return true;
}
