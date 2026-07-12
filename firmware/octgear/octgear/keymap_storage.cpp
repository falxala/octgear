#include "keymap_storage.h"

#include <hardware/flash.h>
#include <hardware/sync.h>

#include "config.h"
#include "key_assignment.h"
#include "keymap.h"

extern uint8_t _FS_start;
extern uint8_t _FS_end;

namespace {

constexpr uint8_t STORAGE_MAGIC[4] = { 'C', 'M', '8', 'J' };
constexpr uint8_t STORAGE_VERSION = 1;
constexpr uint8_t SLOT_COUNT = 3;
constexpr uint32_t SLOT_SIZE = FLASH_SECTOR_SIZE;
constexpr uint8_t ASSIGNMENT_RECORD_SIZE = 11;
constexpr uint8_t LAYER_COLOR_SIZE = 3;

constexpr int GENERATION_ADDRESS = 8;
constexpr int PAYLOAD_LENGTH_ADDRESS = 12;
constexpr int ENCODER_REVERSED_ADDRESS = 14;
constexpr int ENCODER_REVERSED_MARKER_ADDRESS = 15;
constexpr uint8_t ENCODER_REVERSED_MARKER = 0xA5;
constexpr int CRC_ADDRESS = 16;
constexpr int STORAGE_HEADER_SIZE = 20;
constexpr int ASSIGNMENTS_ADDRESS = STORAGE_HEADER_SIZE;
constexpr int ENABLED_LAYER_MASK_ADDRESS =
  ASSIGNMENTS_ADDRESS + (Config::LAYER_COUNT * Config::KEY_COUNT * ASSIGNMENT_RECORD_SIZE);
constexpr int LAYER_COLORS_ADDRESS = ENABLED_LAYER_MASK_ADDRESS + 1;
constexpr int STORAGE_DATA_SIZE = LAYER_COLORS_ADDRESS + (Config::LAYER_COUNT * LAYER_COLOR_SIZE);
constexpr int PROGRAM_SIZE = ((STORAGE_DATA_SIZE + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE) * FLASH_PAGE_SIZE;

static_assert(STORAGE_DATA_SIZE <= static_cast<int>(SLOT_SIZE), "Keymap must fit in one flash sector");
static_assert(PROGRAM_SIZE <= static_cast<int>(SLOT_SIZE), "Program data must fit in one flash sector");

bool storageAvailable = false;
int8_t currentSlot = -1;
uint32_t currentGeneration = 0;

int recordAddress(uint8_t layer, uint8_t keyIndex) {
  const int recordIndex = (layer * Config::KEY_COUNT) + keyIndex;
  return ASSIGNMENTS_ADDRESS + (recordIndex * ASSIGNMENT_RECORD_SIZE);
}

int layerColorAddress(uint8_t layer) {
  return LAYER_COLORS_ADDRESS + (layer * LAYER_COLOR_SIZE);
}

const uint8_t* slotData(uint8_t slot) {
  return reinterpret_cast<const uint8_t*>(&_FS_start) + (slot * SLOT_SIZE);
}

uint32_t slotFlashOffset(uint8_t slot) {
  return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(slotData(slot)) - XIP_BASE);
}

uint16_t readUint16(const uint8_t* data, int address) {
  return static_cast<uint16_t>(data[address]) |
         (static_cast<uint16_t>(data[address + 1]) << 8);
}

uint32_t readUint32(const uint8_t* data, int address) {
  return static_cast<uint32_t>(data[address]) |
         (static_cast<uint32_t>(data[address + 1]) << 8) |
         (static_cast<uint32_t>(data[address + 2]) << 16) |
         (static_cast<uint32_t>(data[address + 3]) << 24);
}

void writeUint16(uint8_t* data, int address, uint16_t value) {
  data[address] = static_cast<uint8_t>(value & 0xFF);
  data[address + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

void writeUint32(uint8_t* data, int address, uint32_t value) {
  data[address] = static_cast<uint8_t>(value & 0xFF);
  data[address + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
  data[address + 2] = static_cast<uint8_t>((value >> 16) & 0xFF);
  data[address + 3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

uint32_t updateCrc32(uint32_t crc, const uint8_t* data, size_t length) {
  for (size_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; bit++) {
      crc = (crc & 1U) != 0 ? (crc >> 1) ^ 0xEDB88320UL : crc >> 1;
    }
  }
  return crc;
}

uint32_t calculateSlotCrc(const uint8_t* data) {
  uint32_t crc = updateCrc32(0xFFFFFFFFUL, data, CRC_ADDRESS);
  crc = updateCrc32(crc, data + STORAGE_HEADER_SIZE, STORAGE_DATA_SIZE - STORAGE_HEADER_SIZE);
  return crc ^ 0xFFFFFFFFUL;
}

bool validKind(uint8_t value) {
  return value <= static_cast<uint8_t>(AssignmentKind::LayerPrevious);
}

bool slotValid(uint8_t slot) {
  const uint8_t* data = slotData(slot);
  for (uint8_t i = 0; i < sizeof(STORAGE_MAGIC); i++) {
    if (data[i] != STORAGE_MAGIC[i]) {
      return false;
    }
  }

  if (data[4] != STORAGE_VERSION ||
      data[5] != Config::LAYER_COUNT ||
      data[6] != Config::KEY_COUNT ||
      data[7] != ASSIGNMENT_RECORD_SIZE ||
      readUint16(data, PAYLOAD_LENGTH_ADDRESS) != STORAGE_DATA_SIZE - STORAGE_HEADER_SIZE) {
    return false;
  }

  return readUint32(data, CRC_ADDRESS) == calculateSlotCrc(data);
}

bool generationIsNewer(uint32_t candidate, uint32_t current) {
  const uint32_t distance = candidate - current;
  return distance != 0 && distance < 0x80000000UL;
}

int8_t findNewestSlot() {
  int8_t newest = -1;
  uint32_t newestGeneration = 0;

  for (uint8_t slot = 0; slot < SLOT_COUNT; slot++) {
    if (!slotValid(slot)) {
      continue;
    }

    const uint32_t generation = readUint32(slotData(slot), GENERATION_ADDRESS);
    if (newest < 0 || generationIsNewer(generation, newestGeneration)) {
      newest = static_cast<int8_t>(slot);
      newestGeneration = generation;
    }
  }

  return newest;
}

bool readAssignmentRecord(const uint8_t* data, int address, KeyAssignment& assignment) {
  const uint8_t kind = data[address];
  if (!validKind(kind)) {
    return false;
  }

  assignment = blankAssignment();
  assignment.kind = static_cast<AssignmentKind>(kind);
  assignment.modifier = data[address + 1];

  for (uint8_t i = 0; i < Config::KEYBOARD_REPORT_SLOTS; i++) {
    assignment.keycodes[i] = data[address + 2 + i];
  }

  assignment.consumerUsage = static_cast<uint16_t>(data[address + 8]) |
                             (static_cast<uint16_t>(data[address + 9]) << 8);
  assignment.targetLayer = data[address + 10];
  return assignment.kind != AssignmentKind::MomentaryLayer ||
         assignment.targetLayer < Config::LAYER_COUNT;
}

void writeAssignmentRecord(uint8_t* data, int address, const KeyAssignment& assignment) {
  data[address] = static_cast<uint8_t>(assignment.kind);
  data[address + 1] = assignment.modifier;

  for (uint8_t i = 0; i < Config::KEYBOARD_REPORT_SLOTS; i++) {
    data[address + 2 + i] = assignment.keycodes[i];
  }

  data[address + 8] = static_cast<uint8_t>(assignment.consumerUsage & 0xFF);
  data[address + 9] = static_cast<uint8_t>((assignment.consumerUsage >> 8) & 0xFF);
  data[address + 10] = assignment.targetLayer;
}

void buildSlotData(uint8_t* data, uint32_t generation) {
  memset(data, 0xFF, PROGRAM_SIZE);
  memcpy(data, STORAGE_MAGIC, sizeof(STORAGE_MAGIC));
  data[4] = STORAGE_VERSION;
  data[5] = Config::LAYER_COUNT;
  data[6] = Config::KEY_COUNT;
  data[7] = ASSIGNMENT_RECORD_SIZE;
  writeUint32(data, GENERATION_ADDRESS, generation);
  writeUint16(data, PAYLOAD_LENGTH_ADDRESS, STORAGE_DATA_SIZE - STORAGE_HEADER_SIZE);
  data[ENCODER_REVERSED_ADDRESS] = encoderReversed() ? 1 : 0;
  data[ENCODER_REVERSED_MARKER_ADDRESS] = ENCODER_REVERSED_MARKER;

  for (uint8_t layer = 0; layer < Config::LAYER_COUNT; layer++) {
    for (uint8_t keyIndex = 0; keyIndex < Config::KEY_COUNT; keyIndex++) {
      writeAssignmentRecord(data, recordAddress(layer, keyIndex), assignmentFor(layer, keyIndex));
    }
  }

  data[ENABLED_LAYER_MASK_ADDRESS] = enabledLayerMask();
  for (uint8_t layer = 0; layer < Config::LAYER_COUNT; layer++) {
    const LayerColor color = layerColor(layer);
    const int address = layerColorAddress(layer);
    data[address] = color.red;
    data[address + 1] = color.green;
    data[address + 2] = color.blue;
  }
  writeUint32(data, CRC_ADDRESS, calculateSlotCrc(data));
}

bool writeSlot(uint8_t slot, const uint8_t* data) {
  noInterrupts();
  rp2040.idleOtherCore();
  flash_range_erase(slotFlashOffset(slot), SLOT_SIZE);
  flash_range_program(slotFlashOffset(slot), data, PROGRAM_SIZE);
  rp2040.resumeOtherCore();
  interrupts();
  return slotValid(slot);
}

}  // namespace

void beginKeymapStorage() {
  const uintptr_t storageSize = reinterpret_cast<uintptr_t>(&_FS_end) -
                                reinterpret_cast<uintptr_t>(&_FS_start);
  storageAvailable = storageSize >= SLOT_COUNT * SLOT_SIZE;
  currentSlot = storageAvailable ? findNewestSlot() : -1;
  currentGeneration = currentSlot >= 0
    ? readUint32(slotData(static_cast<uint8_t>(currentSlot)), GENERATION_ADDRESS)
    : 0;
}

bool loadKeymapFromStorage() {
  if (!storageAvailable || currentSlot < 0) {
    return false;
  }

  const uint8_t* data = slotData(static_cast<uint8_t>(currentSlot));
  KeyAssignment loaded[Config::LAYER_COUNT][Config::KEY_COUNT];
  for (uint8_t layer = 0; layer < Config::LAYER_COUNT; layer++) {
    for (uint8_t keyIndex = 0; keyIndex < Config::KEY_COUNT; keyIndex++) {
      if (!readAssignmentRecord(data, recordAddress(layer, keyIndex), loaded[layer][keyIndex])) {
        return false;
      }
    }
  }

  for (uint8_t layer = 0; layer < Config::LAYER_COUNT; layer++) {
    for (uint8_t keyIndex = 0; keyIndex < Config::KEY_COUNT; keyIndex++) {
      setAssignment(layer, keyIndex, loaded[layer][keyIndex]);
    }
  }

  setEnabledLayerMask(data[ENABLED_LAYER_MASK_ADDRESS]);
  for (uint8_t layer = 0; layer < Config::LAYER_COUNT; layer++) {
    const int address = layerColorAddress(layer);
    setLayerColor(layer, { data[address], data[address + 1], data[address + 2] });
  }
  setEncoderReversed(data[ENCODER_REVERSED_MARKER_ADDRESS] == ENCODER_REVERSED_MARKER
    ? data[ENCODER_REVERSED_ADDRESS] != 0
    : Config::ENCODER_REVERSED);
  return true;
}

bool saveKeymapToStorage() {
  if (!storageAvailable) {
    return false;
  }

  const uint8_t targetSlot = currentSlot < 0
    ? 0
    : static_cast<uint8_t>((currentSlot + 1) % SLOT_COUNT);
  const uint32_t nextGeneration = currentGeneration + 1;
  uint8_t data[PROGRAM_SIZE];
  buildSlotData(data, nextGeneration);

  if (!writeSlot(targetSlot, data)) {
    return false;
  }

  currentSlot = static_cast<int8_t>(targetSlot);
  currentGeneration = nextGeneration;
  return true;
}

bool saveAssignmentToStorage(uint8_t layer, uint8_t keyIndex) {
  if (layer >= Config::LAYER_COUNT || keyIndex >= Config::KEY_COUNT) {
    return false;
  }
  return saveKeymapToStorage();
}

bool saveEnabledLayerMaskToStorage() {
  return saveKeymapToStorage();
}

bool saveLayerColorToStorage(uint8_t layer) {
  if (layer >= Config::LAYER_COUNT) {
    return false;
  }
  return saveKeymapToStorage();
}

bool saveEncoderReversedToStorage() {
  return saveKeymapToStorage();
}

bool runKeymapStorageSelfTest() {
  KeyAssignment backup[Config::LAYER_COUNT][Config::KEY_COUNT];
  KeyAssignment pattern[Config::LAYER_COUNT][Config::KEY_COUNT];
  const uint8_t backupLayerMask = enabledLayerMask();
  const uint8_t backupActiveLayer = activeLayer();
  const bool backupEncoderReversed = encoderReversed();
  const bool patternEncoderReversed = !backupEncoderReversed;
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
  setEncoderReversed(patternEncoderReversed);

  bool ok = saveKeymapToStorage();
  if (ok) {
    clearKeymap();
    setEnabledLayerMask(0x01);
    resetLayerColors();
    setEncoderReversed(backupEncoderReversed);
    ok = loadKeymapFromStorage();
  }

  if (ok && enabledLayerMask() != patternLayerMask) {
    ok = false;
  }

  if (ok && encoderReversed() != patternEncoderReversed) {
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
  setEncoderReversed(backupEncoderReversed);
  for (uint8_t layer = 0; layer < Config::LAYER_COUNT; layer++) {
    setLayerColor(layer, backupLayerColors[layer]);
  }

  const bool restored = saveKeymapToStorage();
  return ok && restored;
}
