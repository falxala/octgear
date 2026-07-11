#include "keymap_storage.h"

#include <EEPROM.h>

#include "config.h"
#include "key_assignment.h"
#include "keymap.h"

namespace {

constexpr uint8_t STORAGE_MAGIC[4] = { 'C', 'M', '8', 'K' };
constexpr uint8_t STORAGE_VERSION = 5;
constexpr uint8_t STORAGE_HEADER_SIZE = 8;
constexpr uint8_t ASSIGNMENT_RECORD_SIZE = 11;
constexpr uint8_t LAYER_COLOR_SIZE = 3;
constexpr int ENABLED_LAYER_MASK_ADDRESS =
  STORAGE_HEADER_SIZE + (Config::LAYER_COUNT * Config::KEY_COUNT * ASSIGNMENT_RECORD_SIZE);
constexpr int LAYER_COLORS_ADDRESS = ENABLED_LAYER_MASK_ADDRESS + 1;
constexpr int STORAGE_SIZE = LAYER_COLORS_ADDRESS + (Config::LAYER_COUNT * LAYER_COLOR_SIZE);

static_assert(STORAGE_SIZE <= 4096, "RP2040 EEPROM emulation supports up to 4096 bytes");

int recordAddress(uint8_t layer, uint8_t keyIndex) {
  const int recordIndex = (layer * Config::KEY_COUNT) + keyIndex;
  return STORAGE_HEADER_SIZE + (recordIndex * ASSIGNMENT_RECORD_SIZE);
}

bool validKind(uint8_t value) {
  return value <= static_cast<uint8_t>(AssignmentKind::LayerPrevious);
}

bool headerMatches() {
  for (uint8_t i = 0; i < sizeof(STORAGE_MAGIC); i++) {
    if (EEPROM.read(i) != STORAGE_MAGIC[i]) {
      return false;
    }
  }

  return EEPROM.read(4) == STORAGE_VERSION &&
         EEPROM.read(5) == Config::LAYER_COUNT &&
         EEPROM.read(6) == Config::KEY_COUNT &&
         EEPROM.read(7) == ASSIGNMENT_RECORD_SIZE;
}

int layerColorAddress(uint8_t layer) {
  return LAYER_COLORS_ADDRESS + (layer * LAYER_COLOR_SIZE);
}

void writeLayerColor(uint8_t layer) {
  const LayerColor color = layerColor(layer);
  const int address = layerColorAddress(layer);
  EEPROM.write(address, color.red);
  EEPROM.write(address + 1, color.green);
  EEPROM.write(address + 2, color.blue);
}

void writeAllLayerColors() {
  for (uint8_t layer = 0; layer < Config::LAYER_COUNT; layer++) {
    writeLayerColor(layer);
  }
}

void writeHeader() {
  for (uint8_t i = 0; i < sizeof(STORAGE_MAGIC); i++) {
    EEPROM.write(i, STORAGE_MAGIC[i]);
  }

  EEPROM.write(4, STORAGE_VERSION);
  EEPROM.write(5, Config::LAYER_COUNT);
  EEPROM.write(6, Config::KEY_COUNT);
  EEPROM.write(7, ASSIGNMENT_RECORD_SIZE);
}

bool readAssignmentRecord(int address, KeyAssignment& assignment) {
  const uint8_t kind = EEPROM.read(address);
  if (!validKind(kind)) {
    return false;
  }

  assignment = blankAssignment();
  assignment.kind = static_cast<AssignmentKind>(kind);
  assignment.modifier = EEPROM.read(address + 1);

  for (uint8_t i = 0; i < Config::KEYBOARD_REPORT_SLOTS; i++) {
    assignment.keycodes[i] = EEPROM.read(address + 2 + i);
  }

  assignment.consumerUsage = static_cast<uint16_t>(EEPROM.read(address + 8)) |
                             (static_cast<uint16_t>(EEPROM.read(address + 9)) << 8);
  assignment.targetLayer = EEPROM.read(address + 10);
  if (assignment.kind == AssignmentKind::MomentaryLayer &&
      assignment.targetLayer >= Config::LAYER_COUNT) {
    return false;
  }

  return true;
}

void writeAssignmentRecord(int address, const KeyAssignment& assignment) {
  EEPROM.write(address, static_cast<uint8_t>(assignment.kind));
  EEPROM.write(address + 1, assignment.modifier);

  for (uint8_t i = 0; i < Config::KEYBOARD_REPORT_SLOTS; i++) {
    EEPROM.write(address + 2 + i, assignment.keycodes[i]);
  }

  EEPROM.write(address + 8, static_cast<uint8_t>(assignment.consumerUsage & 0xFF));
  EEPROM.write(address + 9, static_cast<uint8_t>((assignment.consumerUsage >> 8) & 0xFF));
  EEPROM.write(address + 10, assignment.targetLayer);
}

}  // namespace

void beginKeymapStorage() {
  EEPROM.begin(STORAGE_SIZE);
}

bool loadKeymapFromStorage() {
  if (!headerMatches()) {
    return false;
  }

  KeyAssignment loaded[Config::LAYER_COUNT][Config::KEY_COUNT];

  for (uint8_t layer = 0; layer < Config::LAYER_COUNT; layer++) {
    for (uint8_t keyIndex = 0; keyIndex < Config::KEY_COUNT; keyIndex++) {
      if (!readAssignmentRecord(recordAddress(layer, keyIndex), loaded[layer][keyIndex])) {
        return false;
      }
    }
  }

  for (uint8_t layer = 0; layer < Config::LAYER_COUNT; layer++) {
    for (uint8_t keyIndex = 0; keyIndex < Config::KEY_COUNT; keyIndex++) {
      setAssignment(layer, keyIndex, loaded[layer][keyIndex]);
    }
  }

  setEnabledLayerMask(EEPROM.read(ENABLED_LAYER_MASK_ADDRESS));
  for (uint8_t layer = 0; layer < Config::LAYER_COUNT; layer++) {
    const int address = layerColorAddress(layer);
    setLayerColor(layer, {
      EEPROM.read(address),
      EEPROM.read(address + 1),
      EEPROM.read(address + 2),
    });
  }

  return true;
}

bool saveKeymapToStorage() {
  writeHeader();

  for (uint8_t layer = 0; layer < Config::LAYER_COUNT; layer++) {
    for (uint8_t keyIndex = 0; keyIndex < Config::KEY_COUNT; keyIndex++) {
      writeAssignmentRecord(recordAddress(layer, keyIndex), assignmentFor(layer, keyIndex));
    }
  }

  EEPROM.write(ENABLED_LAYER_MASK_ADDRESS, enabledLayerMask());
  writeAllLayerColors();

  return EEPROM.commit();
}

bool saveAssignmentToStorage(uint8_t layer, uint8_t keyIndex) {
  if (layer >= Config::LAYER_COUNT || keyIndex >= Config::KEY_COUNT) {
    return false;
  }

  if (!headerMatches()) {
    return saveKeymapToStorage();
  }

  writeAssignmentRecord(recordAddress(layer, keyIndex), assignmentFor(layer, keyIndex));
  return EEPROM.commit();
}

bool saveEnabledLayerMaskToStorage() {
  if (!headerMatches()) {
    return saveKeymapToStorage();
  }

  EEPROM.write(ENABLED_LAYER_MASK_ADDRESS, enabledLayerMask());
  return EEPROM.commit();
}

bool saveLayerColorToStorage(uint8_t layer) {
  if (layer >= Config::LAYER_COUNT) {
    return false;
  }

  if (!headerMatches()) {
    return saveKeymapToStorage();
  }

  writeLayerColor(layer);
  return EEPROM.commit();
}

bool runKeymapStorageSelfTest() {
  KeyAssignment backup[Config::LAYER_COUNT][Config::KEY_COUNT];
  KeyAssignment pattern[Config::LAYER_COUNT][Config::KEY_COUNT];
  const uint8_t backupLayerMask = enabledLayerMask();
  const uint8_t backupActiveLayer = activeLayer();
  const uint8_t patternLayerMask = static_cast<uint8_t>(0x55U & ((1U << Config::LAYER_COUNT) - 1U));
  LayerColor backupLayerColors[Config::LAYER_COUNT];
  LayerColor patternLayerColors[Config::LAYER_COUNT];

  for (uint8_t layer = 0; layer < Config::LAYER_COUNT; layer++) {
    backupLayerColors[layer] = layerColor(layer);
    patternLayerColors[layer] = {
      static_cast<uint8_t>(17 + layer),
      static_cast<uint8_t>(83 + layer),
      static_cast<uint8_t>(149 + layer),
    };
    setLayerColor(layer, patternLayerColors[layer]);

    for (uint8_t keyIndex = 0; keyIndex < Config::KEY_COUNT; keyIndex++) {
      backup[layer][keyIndex] = assignmentFor(layer, keyIndex);

      KeyAssignment assignment = blankAssignment();
      assignment.kind = AssignmentKind::Keyboard;
      assignment.modifier = static_cast<uint8_t>((layer << 4) ^ keyIndex);
      for (uint8_t slot = 0; slot < Config::KEYBOARD_REPORT_SLOTS; slot++) {
        assignment.keycodes[slot] = static_cast<uint8_t>(0x04 + ((layer * Config::KEY_COUNT + keyIndex + slot) % 0x40));
      }
      assignment.consumerUsage = static_cast<uint16_t>(0x1200 + (layer * Config::KEY_COUNT) + keyIndex);
      pattern[layer][keyIndex] = assignment;
      setAssignment(layer, keyIndex, assignment);
    }
  }

  setEnabledLayerMask(patternLayerMask);

  bool ok = saveKeymapToStorage();
  if (ok) {
    clearKeymap();
    setEnabledLayerMask(0x01);
    resetLayerColors();
    ok = loadKeymapFromStorage();
  }

  if (ok && enabledLayerMask() != patternLayerMask) {
    ok = false;
  }

  if (ok) {
    for (uint8_t layer = 0; layer < Config::LAYER_COUNT; layer++) {
      const LayerColor actual = layerColor(layer);
      const LayerColor expected = patternLayerColors[layer];
      if (actual.red != expected.red || actual.green != expected.green || actual.blue != expected.blue) {
        ok = false;
        break;
      }
    }
  }

  if (ok) {
    for (uint8_t layer = 0; layer < Config::LAYER_COUNT; layer++) {
      for (uint8_t keyIndex = 0; keyIndex < Config::KEY_COUNT; keyIndex++) {
        const KeyAssignment& actual = assignmentFor(layer, keyIndex);
        const KeyAssignment& expected = pattern[layer][keyIndex];
        if (actual.kind != expected.kind ||
            actual.modifier != expected.modifier ||
            actual.consumerUsage != expected.consumerUsage ||
            actual.targetLayer != expected.targetLayer) {
          ok = false;
          break;
        }

        for (uint8_t slot = 0; slot < Config::KEYBOARD_REPORT_SLOTS; slot++) {
          if (actual.keycodes[slot] != expected.keycodes[slot]) {
            ok = false;
            break;
          }
        }

        if (!ok) {
          break;
        }
      }
    }
  }

  for (uint8_t layer = 0; layer < Config::LAYER_COUNT; layer++) {
    for (uint8_t keyIndex = 0; keyIndex < Config::KEY_COUNT; keyIndex++) {
      setAssignment(layer, keyIndex, backup[layer][keyIndex]);
    }
  }

  setEnabledLayerMask(backupLayerMask);
  setActiveLayer(backupActiveLayer);
  for (uint8_t layer = 0; layer < Config::LAYER_COUNT; layer++) {
    setLayerColor(layer, backupLayerColors[layer]);
  }

  const bool restored = saveKeymapToStorage();
  return ok && restored;
}
