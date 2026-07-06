#pragma once

#include <Arduino.h>
#include "Adafruit_TinyUSB.h"
#include "key_assignment.h"

void queueKeyboardAssignment(uint8_t reportId, const KeyAssignment& assignment);
void queueKeyboardRelease(uint8_t reportId);
void queueAllKeyboardReleases();
void queueConsumerTap(uint16_t usage);
void serviceConsumerReports();
bool flushKeyboardReports(Adafruit_USBD_HID& usbHid);
bool flushConsumerReport(Adafruit_USBD_HID& usbHid);
