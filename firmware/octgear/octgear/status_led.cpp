#include "status_led.h"

#include <Adafruit_NeoPixel.h>

#include "config.h"
#include "keymap.h"

namespace {

uint32_t lastUpdateMs = 0;
uint8_t colorWheelPosition = 0;
bool idleShown = false;
bool previewActive = false;
uint8_t displayedLayer = 0xFF;
uint32_t displayedColor = 0;
Adafruit_NeoPixel statusPixel(
  Config::EXTERNAL_RGB_LED_COUNT,
  Config::STATUS_LED_PIN,
  NEO_GRB + NEO_KHZ800
);
#if defined(PIN_NEOPIXEL)
Adafruit_NeoPixel builtInStatusPixel(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
#endif

void showPixelColor(uint32_t color) {
  displayedColor = color;
  for (uint8_t pixel = 0; pixel < Config::EXTERNAL_RGB_LED_COUNT; ++pixel) {
    statusPixel.setPixelColor(pixel, color);
  }
  statusPixel.show();

#if defined(PIN_NEOPIXEL)
  if (PIN_NEOPIXEL != Config::STATUS_LED_PIN) {
    builtInStatusPixel.setPixelColor(0, color);
    builtInStatusPixel.show();
  }
#endif
}

uint32_t balancedPixelColor(uint8_t red, uint8_t green, uint8_t blue) {
  const uint16_t total = static_cast<uint16_t>(red) + green + blue;
  if (total > 255) {
    red = static_cast<uint8_t>((static_cast<uint32_t>(red) * 255U + total / 2U) / total);
    green = static_cast<uint8_t>((static_cast<uint32_t>(green) * 255U + total / 2U) / total);
    blue = static_cast<uint8_t>((static_cast<uint32_t>(blue) * 255U + total / 2U) / total);
  }

  return statusPixel.Color(red, green, blue);
}

uint32_t colorWheel(uint8_t position) {
  position = 255 - position;

  if (position < 85) {
    return balancedPixelColor(255 - position * 3, 0, position * 3);
  }

  if (position < 170) {
    position -= 85;
    return balancedPixelColor(0, position * 3, 255 - position * 3);
  }

  position -= 170;
  return balancedPixelColor(position * 3, 255 - position * 3, 0);
}

void setLayerColorLed(const LayerColor& color) {
  showPixelColor(balancedPixelColor(color.red, color.green, color.blue));
}

void setKeyboardLayerLed(uint8_t layer) {
  setLayerColorLed(layerColor(layer));
  displayedLayer = layer;
}

void setRescueLed() {
  showPixelColor(balancedPixelColor(0, Config::STATUS_RESCUE_GREEN, 0));
}

}  // namespace

void beginStatusLed() {
  statusPixel.begin();
  statusPixel.setBrightness(statusLedBrightness());

#if defined(PIN_NEOPIXEL)
  if (PIN_NEOPIXEL != Config::STATUS_LED_PIN) {
    builtInStatusPixel.begin();
    builtInStatusPixel.setBrightness(statusLedBrightness());
  }
#endif

  setStatusLed(false);
}

void applyStatusLedBrightness() {
  statusPixel.setBrightness(statusLedBrightness());
#if defined(PIN_NEOPIXEL)
  if (PIN_NEOPIXEL != Config::STATUS_LED_PIN) {
    builtInStatusPixel.setBrightness(statusLedBrightness());
  }
#endif
  showPixelColor(displayedColor);
}

void setStatusLed(bool on) {
  showPixelColor(on ? colorWheel(colorWheelPosition) : 0);

  if (!on) {
    displayedLayer = 0xFF;
  }
}

void previewStatusLedColor(uint8_t red, uint8_t green, uint8_t blue) {
  setLayerColorLed({ red, green, blue });
  previewActive = true;
  idleShown = true;
  displayedLayer = 0xFF;
  lastUpdateMs = 0;
}

void clearStatusLedPreview() {
  previewActive = false;
  idleShown = false;
  displayedLayer = 0xFF;
  lastUpdateMs = 0;
}

void updateStatusHeartbeat(bool mounted, bool remapperConnected, bool rescueActive, uint8_t layer) {
  if (!mounted) {
    previewActive = false;
    setStatusLed(false);
    lastUpdateMs = 0;
    idleShown = false;
    return;
  }

  if (rescueActive) {
    if (previewActive) {
      clearStatusLedPreview();
    }
    if (!idleShown) {
      setRescueLed();
      idleShown = true;
    }
    lastUpdateMs = 0;
    displayedLayer = 0xFF;
    return;
  }

  if (!remapperConnected) {
    if (previewActive) {
      clearStatusLedPreview();
    }
    if (!idleShown || displayedLayer != layer) {
      setKeyboardLayerLed(layer);
      idleShown = true;
    }
    lastUpdateMs = 0;
    return;
  }

  if (previewActive) {
    return;
  }

  idleShown = false;
  displayedLayer = 0xFF;
  const uint32_t now = millis();

  if (lastUpdateMs != 0 && now - lastUpdateMs < Config::STATUS_COLOR_WHEEL_MS) {
    return;
  }

  lastUpdateMs = now;
  colorWheelPosition += 2;
  showPixelColor(colorWheel(colorWheelPosition));
}
