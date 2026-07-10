#include "hid_report_descriptor.h"

#include "config.h"
#include "hid_reports.h"
#include "Adafruit_TinyUSB.h"

namespace {

uint8_t const kHidReportDescriptor[] = {
  TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(RID_KEYBOARD_1)),
  TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(RID_KEYBOARD_2)),
  TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(RID_KEYBOARD_3)),
  TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(RID_KEYBOARD_4)),
  TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(RID_KEYBOARD_5)),
  TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(RID_KEYBOARD_6)),
  TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(RID_KEYBOARD_7)),
  TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(RID_KEYBOARD_8)),
  TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(RID_KEYBOARD_9)),
  TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(RID_KEYBOARD_10)),
  TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(RID_KEYBOARD_11)),
  TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(RID_CONSUMER_CONTROL)),

  0x06, 0x00, 0xFF,                 // Usage Page (Vendor Defined)
  0x09, 0x01,                       // Usage (Vendor Usage 1)
  0xA1, 0x01,                       // Collection (Application)
  0x85, RID_CONFIG,                 // Report ID
  0x15, 0x00,                       // Logical Minimum (0)
  0x26, 0xFF, 0x00,                 // Logical Maximum (255)
  0x75, 0x08,                       // Report Size (8)
  0x95, Config::CONFIG_REPORT_SIZE, // Report Count
  0x09, 0x01,                       // Usage (Vendor Usage 1)
  0x81, 0x02,                       // Input (Data, Variable, Absolute)
  0x95, Config::CONFIG_REPORT_SIZE, // Report Count
  0x09, 0x02,                       // Usage (Vendor Usage 2)
  0x91, 0x02,                       // Output (Data, Variable, Absolute)
  0x95, Config::CONFIG_REPORT_SIZE, // Report Count
  0x09, 0x03,                       // Usage (Vendor Usage 3)
  0xB1, 0x02,                       // Feature (Data, Variable, Absolute)
  0xC0,                             // End Collection
};

}  // namespace

uint8_t const* hidReportDescriptor() {
  return kHidReportDescriptor;
}

size_t hidReportDescriptorSize() {
  return sizeof(kHidReportDescriptor);
}
