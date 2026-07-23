#pragma once

#include <Arduino.h>
#include "generated_hardware_config.h"

namespace Config {

constexpr uint8_t KEYBOARD_REPORT_SLOTS = 6;
constexpr uint8_t CONFIG_REPORT_SIZE = 32;

constexpr uint8_t STATUS_LED_PIN = EXTERNAL_RGB_LED_PIN;
constexpr uint8_t STATUS_RESCUE_GREEN = 18;

constexpr uint16_t DEBOUNCE_US = 5000;
constexpr uint16_t IDLE_SCAN_SLEEP_US = 100;
constexpr uint16_t REMAPPER_SCAN_SLEEP_US = 1000;
constexpr uint16_t STATUS_COLOR_WHEEL_MS = 40;
constexpr uint16_t REMAPPER_HEARTBEAT_TIMEOUT_MS = 3000;
constexpr uint8_t CONFIG_RESPONSE_READY_RETRIES = 20;
constexpr uint16_t CONFIG_RESPONSE_RETRY_DELAY_US = 100;

constexpr bool README_DRIVE_ENABLED = false;
constexpr uint8_t README_DRIVE_ENABLE_KEY_INDEX = 4;
constexpr const char* REMAPPER_URL = "https://falxala.github.io/octgear/";

}  // namespace Config
