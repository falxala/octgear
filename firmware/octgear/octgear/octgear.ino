#include "hid_device.h"
#include "config.h"
#include "key_scanner.h"
#include "keymap.h"
#include "readme_drive.h"
#include "serial_rescue.h"
#include "status_led.h"

#if defined(ARDUINO_ARCH_RP2040)
#include "pico/time.h"
#endif

namespace {

void sleepBetweenScans(bool remapperActive) {
#if defined(ARDUINO_ARCH_RP2040)
  sleep_us(remapperActive ? Config::REMAPPER_SCAN_SLEEP_US : Config::IDLE_SCAN_SLEEP_US);
#else
  (void)remapperActive;
#endif
}

}  // namespace

void setup() {
  beginKeymap();
  beginStatusLed();
  beginKeyScanner();
  const bool rescueBoot = readmeDriveRequestedAtBoot();
  if (rescueBoot) {
    beginSerialRescue();
  }
  beginHidDevice();
}

void loop() {
  const bool remapperActive = remapperConnected();
  const bool readmeActive = readmeDriveActive();
  const bool rescueActive = readmeActive && serialRescueActive();
  const bool rescueIndicatorActive = readmeActive || rescueActive;
  const bool configActive = remapperActive || rescueIndicatorActive;

  if (updateKeyScanner(!configActive) && !rescueIndicatorActive) {
    sendKeyChanges(previousKeyMask(), currentKeyMask(), activeLayer());
  }

  updateHidDevice();
  if (readmeActive) {
    updateSerialRescue();
  }
  updateStatusHeartbeat(hidDeviceMounted(), remapperActive, rescueIndicatorActive, activeLayer());
  sleepBetweenScans(configActive);
}
