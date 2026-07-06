#include "keymap.h"

#include "Adafruit_TinyUSB.h"
#include "keymap_storage.h"

namespace {

uint8_t currentLayer = 0;
KeyAssignment keymap[Config::LAYER_COUNT][Config::KEY_COUNT];

void setDefaultLayer0() {
  keymap[0][0] = layerCycleAssignment();
  keymap[0][1] = consumerAssignment(HID_USAGE_CONSUMER_VOLUME_INCREMENT);
  keymap[0][2] = consumerAssignment(HID_USAGE_CONSUMER_VOLUME_DECREMENT);
  keymap[0][3] = consumerAssignment(HID_USAGE_CONSUMER_MUTE);
  keymap[0][4] = momentaryLayerAssignment(2);
  keymap[0][5] = consumerAssignment(HID_USAGE_CONSUMER_SCAN_PREVIOUS_TRACK);
  keymap[0][6] = consumerAssignment(HID_USAGE_CONSUMER_PLAY_PAUSE);
  keymap[0][7] = consumerAssignment(HID_USAGE_CONSUMER_SCAN_NEXT_TRACK);
}

void setDefaultLayer1() {
  keymap[1][0] = keyboardAssignment(HID_KEY_Q);
  keymap[1][1] = keyboardAssignment(HID_KEY_W);
  keymap[1][2] = keyboardAssignment(HID_KEY_E);
  keymap[1][3] = keyboardAssignment(HID_KEY_R);
  keymap[1][4] = keyboardAssignment(HID_KEY_A);
  keymap[1][5] = keyboardAssignment(HID_KEY_S);
  keymap[1][6] = keyboardAssignment(HID_KEY_D);
  keymap[1][7] = keyboardAssignment(HID_KEY_F);
}

void setDefaultLayer2() {
  keymap[2][0] = keyboardAssignment(HID_KEY_ARROW_UP);
  keymap[2][1] = keyboardAssignment(HID_KEY_ARROW_DOWN);
  keymap[2][2] = keyboardAssignment(HID_KEY_ARROW_LEFT);
  keymap[2][3] = keyboardAssignment(HID_KEY_ARROW_RIGHT);
  keymap[2][4] = keyboardAssignment(HID_KEY_HOME);
  keymap[2][5] = keyboardAssignment(HID_KEY_END);
  keymap[2][6] = keyboardAssignment(HID_KEY_PAGE_UP);
  keymap[2][7] = keyboardAssignment(HID_KEY_PAGE_DOWN);
}

void setDefaultKeymap() {
  clearKeymap();
  setDefaultLayer0();
  setDefaultLayer1();
  setDefaultLayer2();
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
  setDefaultKeymap();
  beginKeymapStorage();

  if (!loadKeymapFromStorage()) {
    setDefaultKeymap();
    saveKeymapToStorage();
  }
}

uint8_t activeLayer() {
  return currentLayer;
}

void setActiveLayer(uint8_t layer) {
  if (layer < Config::LAYER_COUNT) {
    currentLayer = layer;
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
