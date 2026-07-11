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
Adafruit_NeoPixel statusPixel(1, Config::STATUS_LED_PIN, NEO_GRB + NEO_KHZ800);

uint32_t colorWheel(uint8_t position) {
  position = 255 - position;

  if (position < 85) {
    return statusPixel.Color(255 - position * 3, 0, position * 3);
  }

  if (position < 170) {
    position -= 85;
    return statusPixel.Color(0, position * 3, 255 - position * 3);
  }

  position -= 170;
  return statusPixel.Color(position * 3, 255 - position * 3, 0);
}

void setLayerColorLed(const LayerColor& color) {
  if (Config::STATUS_LED_KIND == Config::StatusLedKind::Digital) {
    const bool on = color.red != 0 || color.green != 0 || color.blue != 0;
    digitalWrite(Config::STATUS_LED_PIN, on ? HIGH : LOW);
  } else if (Config::STATUS_LED_KIND == Config::StatusLedKind::NeoPixel) {
    statusPixel.setPixelColor(0, statusPixel.Color(color.red, color.green, color.blue));
    statusPixel.show();
  }
}

void setKeyboardLayerLed(uint8_t layer) {
  setLayerColorLed(layerColor(layer));
  displayedLayer = layer;
}

void setRescueLed() {
  if (Config::STATUS_LED_KIND == Config::StatusLedKind::Digital) {
    digitalWrite(Config::STATUS_LED_PIN, HIGH);
  } else if (Config::STATUS_LED_KIND == Config::StatusLedKind::NeoPixel) {
    statusPixel.setPixelColor(0, statusPixel.Color(0, Config::STATUS_RESCUE_GREEN, 0));
    statusPixel.show();
  }
}

}  // namespace

void beginStatusLed() {
  if (Config::STATUS_LED_KIND == Config::StatusLedKind::Digital) {
    pinMode(Config::STATUS_LED_PIN, OUTPUT);
  } else if (Config::STATUS_LED_KIND == Config::StatusLedKind::NeoPixel) {
    statusPixel.begin();
    statusPixel.setBrightness(Config::STATUS_LED_BRIGHTNESS);
  }

  setStatusLed(false);
}

void setStatusLed(bool on) {
  if (Config::STATUS_LED_KIND == Config::StatusLedKind::Digital) {
    digitalWrite(Config::STATUS_LED_PIN, on ? HIGH : LOW);
  } else if (Config::STATUS_LED_KIND == Config::StatusLedKind::NeoPixel) {
    statusPixel.setPixelColor(0, on ? colorWheel(colorWheelPosition) : 0);
    statusPixel.show();
  }

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

  if (Config::STATUS_LED_KIND == Config::StatusLedKind::NeoPixel) {
    if (lastUpdateMs != 0 && now - lastUpdateMs < Config::STATUS_COLOR_WHEEL_MS) {
      return;
    }

    lastUpdateMs = now;
    colorWheelPosition += 2;
    statusPixel.setPixelColor(0, colorWheel(colorWheelPosition));
    statusPixel.show();
    return;
  }

  if (Config::STATUS_LED_KIND == Config::StatusLedKind::Digital) {
    setStatusLed(true);
  }
}
