#include "keymap_storage.h"

#include <EEPROM.h>

#include "config.h"
#include "key_assignment.h"
#include "keymap.h"

namespace {

constexpr uint8_t STORAGE_MAGIC[4] = { 'C', 'M', '8', 'K' };
constexpr uint8_t STORAGE_VERSION = 4;
constexpr uint8_t STORAGE_HEADER_SIZE = 8;
constexpr uint8_t ASSIGNMENT_RECORD_SIZE = 11;
constexpr int STORAGE_SIZE =
  STORAGE_HEADER_SIZE + (Config::LAYER_COUNT * Config::KEY_COUNT * ASSIGNMENT_RECORD_SIZE);

static_assert(STORAGE_SIZE <= 4096, "RP2040 EEPROM emulation supports up to 4096 bytes");

int recordAddress(uint8_t layer, uint8_t keyIndex) {
  const int recordIndex = (layer * Config::KEY_COUNT) + keyIndex;
  return STORAGE_HEADER_SIZE + (recordIndex * ASSIGNMENT_RECORD_SIZE);
}

bool validKind(uint8_t value) {
  return value <= static_cast<uint8_t>(AssignmentKind::MomentaryLayer);
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

  return true;
}

bool saveKeymapToStorage() {
  writeHeader();

  for (uint8_t layer = 0; layer < Config::LAYER_COUNT; layer++) {
    for (uint8_t keyIndex = 0; keyIndex < Config::KEY_COUNT; keyIndex++) {
      writeAssignmentRecord(recordAddress(layer, keyIndex), assignmentFor(layer, keyIndex));
    }
  }

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

bool runKeymapStorageSelfTest() {
  KeyAssignment backup[Config::LAYER_COUNT][Config::KEY_COUNT];
  KeyAssignment pattern[Config::LAYER_COUNT][Config::KEY_COUNT];

  for (uint8_t layer = 0; layer < Config::LAYER_COUNT; layer++) {
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

  bool ok = saveKeymapToStorage();
  if (ok) {
    clearKeymap();
    ok = loadKeymapFromStorage();
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

  const bool restored = saveKeymapToStorage();
  return ok && restored;
}
