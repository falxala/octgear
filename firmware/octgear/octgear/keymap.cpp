#include "keymap.h"

#include "Adafruit_TinyUSB.h"
#include "keymap_storage.h"

namespace {

uint8_t currentLayer = 0;
static_assert(Config::LAYER_COUNT > 0 && Config::LAYER_COUNT <= 8, "Layer mask supports 1-8 layers");
constexpr uint8_t ALL_LAYERS_ENABLED_MASK = static_cast<uint8_t>((1U << Config::LAYER_COUNT) - 1U);
uint8_t currentEnabledLayerMask = Config::DEFAULT_ENABLED_LAYER_MASK;
bool currentEncoderReversed = Config::ENCODER_REVERSED;
uint8_t currentStatusLedBrightness = Config::DEFAULT_STATUS_LED_BRIGHTNESS;
LayerColor currentLayerColors[Config::LAYER_COUNT];
KeyAssignment keymap[Config::LAYER_COUNT][Config::KEY_COUNT];

void setDefaultLayer0() {
  keymap[0][0] = layerCycleAssignment();
  keymap[0][1] = consumerAssignment(HID_USAGE_CONSUMER_MUTE);
  keymap[0][2] = consumerAssignment(HID_USAGE_CONSUMER_VOLUME_DECREMENT);
  keymap[0][3] = consumerAssignment(HID_USAGE_CONSUMER_VOLUME_INCREMENT);
  keymap[0][4] = momentaryLayerAssignment(1);
  keymap[0][5] = consumerAssignment(HID_USAGE_CONSUMER_SCAN_PREVIOUS_TRACK);
  keymap[0][6] = consumerAssignment(HID_USAGE_CONSUMER_PLAY_PAUSE);
  keymap[0][7] = consumerAssignment(HID_USAGE_CONSUMER_SCAN_NEXT_TRACK);
  keymap[0][Config::ENCODER_CCW_KEY_INDEX] = consumerAssignment(HID_USAGE_CONSUMER_VOLUME_DECREMENT);
  keymap[0][Config::ENCODER_CW_KEY_INDEX] = consumerAssignment(HID_USAGE_CONSUMER_VOLUME_INCREMENT);
}

void setDefaultLayer1() {
  keymap[1][0] = layerCycleAssignment();
  keymap[1][1] = keyboardAssignment(HID_KEY_Q);
  keymap[1][2] = keyboardAssignment(HID_KEY_W);
  keymap[1][3] = keyboardAssignment(HID_KEY_E);
  keymap[1][5] = keyboardAssignment(HID_KEY_A);
  keymap[1][6] = keyboardAssignment(HID_KEY_S);
  keymap[1][7] = keyboardAssignment(HID_KEY_D);
}

void setDefaultKeymap() {
  clearKeymap();
  setDefaultLayer0();
  setDefaultLayer1();
}

}  // namespace

void clearKeymap() {
  for (uint8_t layer = 0; layer < Config::LAYER_COUNT; layer++) {
    for (uint8_t key = 0; key < Config::KEY_COUNT; key++) {
      keymap[layer][key] = blankAssignment();
    }
  }
}

void beginKeymap() {
  resetKeymapToDefaults();
  beginKeymapStorage();

  if (!loadKeymapFromStorage()) {
    saveKeymapToStorage();
  }
}

void resetKeymapToDefaults() {
  currentLayer = 0;
  currentEnabledLayerMask = Config::DEFAULT_ENABLED_LAYER_MASK;
  currentEncoderReversed = Config::ENCODER_REVERSED;
  currentStatusLedBrightness = Config::DEFAULT_STATUS_LED_BRIGHTNESS;
  resetLayerColors();
  setDefaultKeymap();
}

LayerColor layerColor(uint8_t layer) {
  if (layer >= Config::LAYER_COUNT) {
    layer = 0;
  }

  return currentLayerColors[layer];
}

bool setLayerColor(uint8_t layer, const LayerColor& color) {
  if (layer >= Config::LAYER_COUNT) {
    return false;
  }

  currentLayerColors[layer] = color;
  return true;
}

void resetLayerColors() {
  for (uint8_t layer = 0; layer < Config::LAYER_COUNT; layer++) {
    currentLayerColors[layer] = {
      Config::DEFAULT_LAYER_COLORS[layer][0],
      Config::DEFAULT_LAYER_COLORS[layer][1],
      Config::DEFAULT_LAYER_COLORS[layer][2],
    };
  }
}

bool encoderReversed() {
  return currentEncoderReversed;
}

void setEncoderReversed(bool reversed) {
  currentEncoderReversed = reversed;
}

uint8_t statusLedBrightness() {
  return currentStatusLedBrightness;
}

void setStatusLedBrightness(uint8_t brightness) {
  currentStatusLedBrightness = brightness > Config::MAX_STATUS_LED_BRIGHTNESS
    ? Config::MAX_STATUS_LED_BRIGHTNESS
    : brightness;
}

uint8_t activeLayer() {
  return currentLayer;
}

void setActiveLayer(uint8_t layer) {
  if (layerEnabled(layer)) {
    currentLayer = layer;
  }
}

uint8_t enabledLayerMask() {
  return currentEnabledLayerMask;
}

bool layerEnabled(uint8_t layer) {
  return layer < Config::LAYER_COUNT &&
         (currentEnabledLayerMask & static_cast<uint8_t>(1U << layer)) != 0;
}

bool setLayerEnabled(uint8_t layer, bool enabled) {
  if (layer >= Config::LAYER_COUNT || (layer == 0 && !enabled)) {
    return false;
  }

  const uint8_t bit = static_cast<uint8_t>(1U << layer);
  currentEnabledLayerMask = enabled
    ? static_cast<uint8_t>(currentEnabledLayerMask | bit)
    : static_cast<uint8_t>(currentEnabledLayerMask & ~bit);

  if (!layerEnabled(currentLayer)) {
    currentLayer = 0;
  }

  return true;
}

void setEnabledLayerMask(uint8_t mask) {
  currentEnabledLayerMask = static_cast<uint8_t>((mask & ALL_LAYERS_ENABLED_MASK) | 0x01U);
  if (!layerEnabled(currentLayer)) {
    currentLayer = 0;
  }
}

const KeyAssignment& assignmentFor(uint8_t layer, uint8_t keyIndex) {
  static KeyAssignment blank = blankAssignment();

  if (layer >= Config::LAYER_COUNT || keyIndex >= Config::KEY_COUNT) {
    return blank;
  }

  return keymap[layer][keyIndex];
}

bool setAssignment(uint8_t layer, uint8_t keyIndex, const KeyAssignment& assignment) {
  if (layer >= Config::LAYER_COUNT || keyIndex >= Config::KEY_COUNT) {
    return false;
  }

  keymap[layer][keyIndex] = assignment;
  return true;
}
