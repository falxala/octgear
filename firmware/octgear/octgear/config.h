#pragma once

#include <Arduino.h>

namespace Config {

constexpr uint8_t KEY_COUNT = 8;
constexpr uint8_t LAYER_COUNT = 6;
constexpr uint8_t KEYBOARD_REPORT_SLOTS = 6;
constexpr uint8_t VIRTUAL_GROUND_COUNT = 2;
constexpr uint8_t CONFIG_REPORT_SIZE = 32;

constexpr uint8_t KEY_PINS[KEY_COUNT] = {
  7, 6, 5, 4, 12, 11, 10, 9
};

// These pins are driven LOW and used as virtual ground rails.
constexpr uint8_t VIRTUAL_GROUND_PINS[VIRTUAL_GROUND_COUNT] = {
  1, 8
};

enum class StatusLedKind : uint8_t {
  None,
  Digital,
  NeoPixel,
};

struct StatusLedColor {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

#if defined(PIN_NEOPIXEL)
constexpr StatusLedKind STATUS_LED_KIND = StatusLedKind::NeoPixel;
constexpr uint8_t STATUS_LED_PIN = PIN_NEOPIXEL;
#elif defined(LED_BUILTIN)
constexpr StatusLedKind STATUS_LED_KIND = StatusLedKind::Digital;
constexpr uint8_t STATUS_LED_PIN = LED_BUILTIN;
#elif defined(PIN_LED)
constexpr StatusLedKind STATUS_LED_KIND = StatusLedKind::Digital;
constexpr uint8_t STATUS_LED_PIN = PIN_LED;
#else
constexpr StatusLedKind STATUS_LED_KIND = StatusLedKind::None;
constexpr uint8_t STATUS_LED_PIN = 255;
#endif

constexpr uint8_t STATUS_LED_BRIGHTNESS = 200;
constexpr uint8_t STATUS_RESCUE_GREEN = 18;
constexpr StatusLedColor STATUS_LAYER_COLORS[LAYER_COUNT] = {
  { 255, 255, 255 },
  { 255, 0, 0 },
  { 0, 180, 255 },
  { 0, 255, 80 },
  { 255, 160, 0 },
  { 180, 0, 255 },
};

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
