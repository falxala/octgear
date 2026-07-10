#include "serial_rescue.h"

#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "key_assignment.h"
#include "keymap.h"
#include "keymap_storage.h"

namespace {

constexpr uint32_t SERIAL_BAUD = 115200;
constexpr uint16_t LINE_BUFFER_SIZE = 96;
constexpr uint32_t RESCUE_ACTIVE_TIMEOUT_MS = 5000;

char lineBuffer[LINE_BUFFER_SIZE];
uint8_t lineLength = 0;
uint32_t lastSerialCommandMs = 0;

char* nextToken(char*& cursor) {
  while (*cursor == ' ' || *cursor == '\t') {
    cursor++;
  }

  if (*cursor == '\0') {
    return nullptr;
  }

  char* token = cursor;
  while (*cursor != '\0' && *cursor != ' ' && *cursor != '\t') {
    cursor++;
  }

  if (*cursor != '\0') {
    *cursor = '\0';
    cursor++;
  }

  return token;
}

bool parseByte(char*& cursor, uint8_t& value) {
  char* token = nextToken(cursor);
  if (token == nullptr) {
    return false;
  }

  char* end = nullptr;
  const unsigned long parsed = strtoul(token, &end, 0);
  if (*end != '\0' || parsed > 0xff) {
    return false;
  }

  value = static_cast<uint8_t>(parsed);
  return true;
}

bool parseWord(char*& cursor, uint16_t& value) {
  char* token = nextToken(cursor);
  if (token == nullptr) {
    return false;
  }

  char* end = nullptr;
  const unsigned long parsed = strtoul(token, &end, 0);
  if (*end != '\0' || parsed > 0xffff) {
    return false;
  }

  value = static_cast<uint16_t>(parsed);
  return true;
}

void printAssignment(uint8_t layer, uint8_t keyIndex) {
  const KeyAssignment& assignment = assignmentFor(layer, keyIndex);
  Serial.print(F("L"));
  Serial.print(layer);
  Serial.print(F(" K"));
  Serial.print(keyIndex + 1);
  Serial.print(F(" kind="));
  Serial.print(static_cast<uint8_t>(assignment.kind));
  Serial.print(F(" mod=0x"));
  if (assignment.modifier < 0x10) {
    Serial.print('0');
  }
  Serial.print(assignment.modifier, HEX);
  Serial.print(F(" keys="));
  for (uint8_t slot = 0; slot < Config::KEYBOARD_REPORT_SLOTS; slot++) {
    if (slot > 0) {
      Serial.print(',');
    }
    Serial.print(F("0x"));
    if (assignment.keycodes[slot] < 0x10) {
      Serial.print('0');
    }
    Serial.print(assignment.keycodes[slot], HEX);
  }
  Serial.print(F(" consumer=0x"));
  if (assignment.consumerUsage < 0x1000) {
    Serial.print('0');
  }
  if (assignment.consumerUsage < 0x100) {
    Serial.print('0');
  }
  if (assignment.consumerUsage < 0x10) {
    Serial.print('0');
  }
  Serial.print(assignment.consumerUsage, HEX);
  Serial.print(F(" targetLayer="));
  Serial.println(assignment.targetLayer);
}

bool parseLayerKey(char*& cursor, uint8_t& layer, uint8_t& keyIndex) {
  uint8_t keyNumber = 0;
  if (!parseByte(cursor, layer) || !parseByte(cursor, keyNumber)) {
    return false;
  }

  if (layer >= Config::LAYER_COUNT || keyNumber == 0 || keyNumber > Config::KEY_COUNT) {
    return false;
  }

  keyIndex = keyNumber - 1;
  return true;
}

void printHelp() {
  Serial.println(F("OctGear serial rescue"));
  Serial.println(F("Commands:"));
  Serial.println(F("  probe"));
  Serial.println(F("  help"));
  Serial.println(F("  state"));
  Serial.println(F("  dump"));
  Serial.println(F("  layer <0-5>"));
  Serial.print(F("  get <layer> <key 1-"));
  Serial.print(Config::KEY_COUNT);
  Serial.println(F(">"));
  Serial.print(F("  none <layer> <key 1-"));
  Serial.print(Config::KEY_COUNT);
  Serial.println(F(">"));
  Serial.print(F("  key <layer> <key 1-"));
  Serial.print(Config::KEY_COUNT);
  Serial.println(F("> <modifier> <keycode> [keycode...]"));
  Serial.print(F("  consumer <layer> <key 1-"));
  Serial.print(Config::KEY_COUNT);
  Serial.println(F("> <usage>"));
  Serial.print(F("  cycle <layer> <key 1-"));
  Serial.print(Config::KEY_COUNT);
  Serial.println(F(">"));
  Serial.print(F("  hold <layer> <key 1-"));
  Serial.print(Config::KEY_COUNT);
  Serial.println(F("> <target layer>"));
  Serial.println(F("  diag"));
  Serial.println(F("  bootloader"));
  Serial.println(F("Numbers accept decimal or 0xHEX. key/none/consumer/cycle/hold save immediately."));
}

void handleState() {
  Serial.print(F("activeLayer="));
  Serial.print(activeLayer());
  Serial.print(F(" layers="));
  Serial.print(Config::LAYER_COUNT);
  Serial.print(F(" keys="));
  Serial.println(Config::KEY_COUNT);
}

void handleProbe() {
  Serial.print(F("OCTGEAR ACK rescue=1 layers="));
  Serial.print(Config::LAYER_COUNT);
  Serial.print(F(" keys="));
  Serial.println(Config::KEY_COUNT);
}

void handleDump() {
  for (uint8_t layer = 0; layer < Config::LAYER_COUNT; layer++) {
    for (uint8_t keyIndex = 0; keyIndex < Config::KEY_COUNT; keyIndex++) {
      printAssignment(layer, keyIndex);
    }
  }
}

void handleLayer(char*& cursor) {
  uint8_t layer = 0;
  if (!parseByte(cursor, layer) || layer >= Config::LAYER_COUNT) {
    Serial.println(F("ERR layer"));
    return;
  }

  setActiveLayer(layer);
  Serial.println(F("OK"));
}

void handleGet(char*& cursor) {
  uint8_t layer = 0;
  uint8_t keyIndex = 0;
  if (!parseLayerKey(cursor, layer, keyIndex)) {
    Serial.println(F("ERR get"));
    return;
  }

  printAssignment(layer, keyIndex);
}

void saveParsedAssignment(uint8_t layer, uint8_t keyIndex, const KeyAssignment& assignment) {
  if (!setAssignment(layer, keyIndex, assignment)) {
    Serial.println(F("ERR range"));
    return;
  }

  if (!saveAssignmentToStorage(layer, keyIndex)) {
    Serial.println(F("ERR storage"));
    return;
  }

  Serial.println(F("OK"));
}

void handleNone(char*& cursor) {
  uint8_t layer = 0;
  uint8_t keyIndex = 0;
  if (!parseLayerKey(cursor, layer, keyIndex)) {
    Serial.println(F("ERR none"));
    return;
  }

  saveParsedAssignment(layer, keyIndex, blankAssignment());
}

void handleKey(char*& cursor) {
  uint8_t layer = 0;
  uint8_t keyIndex = 0;
  uint8_t modifier = 0;
  if (!parseLayerKey(cursor, layer, keyIndex) || !parseByte(cursor, modifier)) {
    Serial.println(F("ERR key"));
    return;
  }

  KeyAssignment assignment = blankAssignment();
  assignment.kind = AssignmentKind::Keyboard;
  assignment.modifier = modifier;

  uint8_t slot = 0;
  uint8_t keycode = 0;
  while (slot < Config::KEYBOARD_REPORT_SLOTS && parseByte(cursor, keycode)) {
    assignment.keycodes[slot] = keycode;
    slot++;
  }

  if (slot == 0) {
    Serial.println(F("ERR keycode"));
    return;
  }

  saveParsedAssignment(layer, keyIndex, assignment);
}

void handleConsumer(char*& cursor) {
  uint8_t layer = 0;
  uint8_t keyIndex = 0;
  uint16_t usage = 0;
  if (!parseLayerKey(cursor, layer, keyIndex) || !parseWord(cursor, usage)) {
    Serial.println(F("ERR consumer"));
    return;
  }

  saveParsedAssignment(layer, keyIndex, consumerAssignment(usage));
}

void handleCycle(char*& cursor) {
  uint8_t layer = 0;
  uint8_t keyIndex = 0;
  if (!parseLayerKey(cursor, layer, keyIndex)) {
    Serial.println(F("ERR cycle"));
    return;
  }

  saveParsedAssignment(layer, keyIndex, layerCycleAssignment());
}

void handleHold(char*& cursor) {
  uint8_t layer = 0;
  uint8_t keyIndex = 0;
  uint8_t targetLayer = 0;
  if (!parseLayerKey(cursor, layer, keyIndex) || !parseByte(cursor, targetLayer) || targetLayer >= Config::LAYER_COUNT) {
    Serial.println(F("ERR hold"));
    return;
  }

  saveParsedAssignment(layer, keyIndex, momentaryLayerAssignment(targetLayer));
}

void handleDiag() {
  Serial.println(runKeymapStorageSelfTest() ? F("OK storage") : F("ERR storage"));
}

void handleBootloader() {
  Serial.println(F("OK bootloader"));
  Serial.flush();
  delay(100);
#if defined(ARDUINO_ARCH_RP2040)
  rp2040.rebootToBootloader();
#else
  Serial.println(F("ERR unsupported"));
#endif
}

void handleCommand(char* line) {
  char* cursor = line;
  char* command = nextToken(cursor);
  if (command == nullptr) {
    return;
  }

  if (strcmp(command, "probe") == 0) {
    handleProbe();
  } else if (strcmp(command, "help") == 0 || strcmp(command, "?") == 0) {
    printHelp();
  } else if (strcmp(command, "state") == 0) {
    handleState();
  } else if (strcmp(command, "dump") == 0) {
    handleDump();
  } else if (strcmp(command, "layer") == 0) {
    handleLayer(cursor);
  } else if (strcmp(command, "get") == 0) {
    handleGet(cursor);
  } else if (strcmp(command, "none") == 0) {
    handleNone(cursor);
  } else if (strcmp(command, "key") == 0) {
    handleKey(cursor);
  } else if (strcmp(command, "consumer") == 0) {
    handleConsumer(cursor);
  } else if (strcmp(command, "cycle") == 0) {
    handleCycle(cursor);
  } else if (strcmp(command, "hold") == 0) {
    handleHold(cursor);
  } else if (strcmp(command, "diag") == 0) {
    handleDiag();
  } else if (strcmp(command, "bootloader") == 0) {
    handleBootloader();
  } else {
    Serial.println(F("ERR unknown"));
  }
  Serial.flush();
}

void printPrompt() {
  Serial.print(F("octgear> "));
  Serial.flush();
}

}  // namespace

void beginSerialRescue() {
  Serial.begin(SERIAL_BAUD);
}

void updateSerialRescue() {
  yield();

  while (Serial.available() > 0) {
    const char ch = static_cast<char>(Serial.read());
    if (ch == '\r') {
      continue;
    }

    if (ch == '\n') {
      lineBuffer[lineLength] = '\0';
      if (lineLength > 0) {
        lastSerialCommandMs = millis();
        handleCommand(lineBuffer);
      }
      lineLength = 0;
      if (lastSerialCommandMs != 0) {
        printPrompt();
      }
      continue;
    }

    if (lineLength < LINE_BUFFER_SIZE - 1) {
      lineBuffer[lineLength] = ch;
      lineLength++;
    } else {
      lineLength = 0;
      Serial.println(F("ERR line too long"));
      printPrompt();
    }
  }
}

bool serialRescueActive() {
  return lastSerialCommandMs != 0 && millis() - lastSerialCommandMs <= RESCUE_ACTIVE_TIMEOUT_MS;
}
