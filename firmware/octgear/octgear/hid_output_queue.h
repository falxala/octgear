#pragma once

#include <Arduino.h>
#include "Adafruit_TinyUSB.h"
#include "key_assignment.h"

void queueKeyboardAssignment(uint8_t keyIndex, const KeyAssignment& assignment);
void queueKeyboardRelease(uint8_t keyIndex);
void queueAllKeyboardReleases();
void queueConsumerPress(uint8_t keyIndex, uint16_t usage);
void queueConsumerRelease(uint8_t keyIndex);
void queueAllConsumerReleases();
void queueConsumerTap(uint16_t usage);
void serviceConsumerReports();
bool hidOutputQueueIdle();
bool flushKeyboardReport(Adafruit_USBD_HID& usbHid);
bool flushConsumerReport(Adafruit_USBD_HID& usbHid);
