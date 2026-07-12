#include "hid_device.h"

#include "Adafruit_TinyUSB.h"
#include "config.h"
#include "hid_output_queue.h"
#include "hid_report_descriptor.h"
#include "hid_reports.h"
#include "key_assignment.h"
#include "keymap.h"
#include "keymap_storage.h"
#include "readme_drive.h"
#include "status_led.h"

bool remapperConnected();

namespace {

Adafruit_USBD_HID usbHid(
  hidReportDescriptor(),
  hidReportDescriptorSize(),
  HID_ITF_PROTOCOL_KEYBOARD,
  2,
  false
);
uint32_t lastRemapperHeartbeatMs = 0;
bool wakeKeyChangePending = false;
Config::KeyMask wakeOldMask = 0;
Config::KeyMask wakeNewMask = 0;
uint8_t wakeLayer = 0;
bool momentaryLayerActive[Config::KEY_COUNT] = { false };
uint8_t momentaryLayerTargets[Config::KEY_COUNT] = { 0 };
uint32_t momentaryLayerOrder[Config::KEY_COUNT] = { 0 };
uint32_t nextMomentaryLayerOrder = 1;
uint8_t momentaryBaseLayer = 0;

bool sendConfigReportWhenReady(const uint8_t* report, uint8_t length) {
  for (uint8_t attempt = 0; attempt < Config::CONFIG_RESPONSE_READY_RETRIES; attempt++) {
    if (usbHid.ready()) {
      usbHid.sendReport(RID_CONFIG, report, length);
      return true;
    }

    delayMicroseconds(Config::CONFIG_RESPONSE_RETRY_DELAY_US);
  }

  return false;
}

void sendConfigResponse(ConfigCommand command, ConfigStatus status, const uint8_t* payload, uint8_t length) {
  uint8_t report[Config::CONFIG_REPORT_SIZE] = { 0 };
  report[0] = static_cast<uint8_t>(command);
  report[1] = static_cast<uint8_t>(status);
  report[2] = length;
  if (report[2] > Config::CONFIG_REPORT_SIZE - 3) {
    report[2] = Config::CONFIG_REPORT_SIZE - 3;
  }

  for (uint8_t i = 0; i < report[2]; i++) {
    report[3 + i] = payload[i];
  }

  sendConfigReportWhenReady(report, sizeof(report));
}

void handleGetState() {
  const uint8_t payload[] = {
    activeLayer(),
    Config::LAYER_COUNT,
    Config::KEY_COUNT,
    Config::MATRIX_ROW_COUNT,
    enabledLayerMask(),
    static_cast<uint8_t>(encoderReversed() ? 1 : 0),
  };

  sendConfigResponse(ConfigCommand::GetState, ConfigStatus::Ok, payload, sizeof(payload));
}

void handleSetLayer(const uint8_t* buffer, uint16_t size) {
  if (size < 2) {
    sendConfigResponse(ConfigCommand::SetLayer, ConfigStatus::InvalidLength, nullptr, 0);
    return;
  }

  const uint8_t layer = buffer[1];
  if (!layerEnabled(layer)) {
    sendConfigResponse(ConfigCommand::SetLayer, ConfigStatus::OutOfRange, nullptr, 0);
    return;
  }

  setActiveLayer(layer);
  clearStatusLedPreview();
  sendConfigResponse(ConfigCommand::SetLayer, ConfigStatus::Ok, &layer, 1);
}

void handleSetLayerEnabled(const uint8_t* buffer, uint16_t size) {
  if (size < 3) {
    sendConfigResponse(ConfigCommand::SetLayerEnabled, ConfigStatus::InvalidLength, nullptr, 0);
    return;
  }

  const uint8_t layer = buffer[1];
  const bool enabled = buffer[2] != 0;
  const uint8_t previousMask = enabledLayerMask();
  const uint8_t previousActiveLayer = activeLayer();
  if (!setLayerEnabled(layer, enabled)) {
    sendConfigResponse(ConfigCommand::SetLayerEnabled, ConfigStatus::OutOfRange, nullptr, 0);
    return;
  }

  if (!saveEnabledLayerMaskToStorage()) {
    setEnabledLayerMask(previousMask);
    setActiveLayer(previousActiveLayer);
    sendConfigResponse(ConfigCommand::SetLayerEnabled, ConfigStatus::StorageError, nullptr, 0);
    return;
  }

  const uint8_t payload[] = {
    layer,
    static_cast<uint8_t>(enabled ? 1 : 0),
    activeLayer(),
    enabledLayerMask(),
  };
  sendConfigResponse(ConfigCommand::SetLayerEnabled, ConfigStatus::Ok, payload, sizeof(payload));
}

void handleGetLayerColor(const uint8_t* buffer, uint16_t size) {
  if (size < 2) {
    sendConfigResponse(ConfigCommand::GetLayerColor, ConfigStatus::InvalidLength, nullptr, 0);
    return;
  }

  const uint8_t layer = buffer[1];
  if (layer >= Config::LAYER_COUNT) {
    sendConfigResponse(ConfigCommand::GetLayerColor, ConfigStatus::OutOfRange, nullptr, 0);
    return;
  }

  const LayerColor color = layerColor(layer);
  const uint8_t payload[] = { layer, color.red, color.green, color.blue };
  sendConfigResponse(ConfigCommand::GetLayerColor, ConfigStatus::Ok, payload, sizeof(payload));
}

void handleSetLayerColor(const uint8_t* buffer, uint16_t size) {
  if (size < 5) {
    sendConfigResponse(ConfigCommand::SetLayerColor, ConfigStatus::InvalidLength, nullptr, 0);
    return;
  }

  const uint8_t layer = buffer[1];
  const LayerColor previousColor = layerColor(layer);
  const LayerColor color = { buffer[2], buffer[3], buffer[4] };
  if (!setLayerColor(layer, color)) {
    sendConfigResponse(ConfigCommand::SetLayerColor, ConfigStatus::OutOfRange, nullptr, 0);
    return;
  }

  if (!saveLayerColorToStorage(layer)) {
    setLayerColor(layer, previousColor);
    sendConfigResponse(ConfigCommand::SetLayerColor, ConfigStatus::StorageError, nullptr, 0);
    return;
  }

  clearStatusLedPreview();
  const uint8_t payload[] = { layer, color.red, color.green, color.blue };
  sendConfigResponse(ConfigCommand::SetLayerColor, ConfigStatus::Ok, payload, sizeof(payload));
}

void handlePreviewLayerColor(const uint8_t* buffer, uint16_t size) {
  if (size == 1) {
    clearStatusLedPreview();
    sendConfigResponse(ConfigCommand::PreviewLayerColor, ConfigStatus::Ok, nullptr, 0);
    return;
  }

  if (size < 5) {
    sendConfigResponse(ConfigCommand::PreviewLayerColor, ConfigStatus::InvalidLength, nullptr, 0);
    return;
  }

  const uint8_t layer = buffer[1];
  if (layer >= Config::LAYER_COUNT) {
    sendConfigResponse(ConfigCommand::PreviewLayerColor, ConfigStatus::OutOfRange, nullptr, 0);
    return;
  }

  previewStatusLedColor(buffer[2], buffer[3], buffer[4]);
  const uint8_t payload[] = { layer, buffer[2], buffer[3], buffer[4] };
  sendConfigResponse(ConfigCommand::PreviewLayerColor, ConfigStatus::Ok, payload, sizeof(payload));
}

void handleResetConfiguration() {
  resetKeymapToDefaults();
  clearStatusLedPreview();

  if (!saveKeymapToStorage()) {
    sendConfigResponse(ConfigCommand::ResetConfiguration, ConfigStatus::StorageError, nullptr, 0);
    return;
  }

  const uint8_t payload[] = { activeLayer(), enabledLayerMask() };
  sendConfigResponse(ConfigCommand::ResetConfiguration, ConfigStatus::Ok, payload, sizeof(payload));
}

void handleSetEncoderReversed(const uint8_t* buffer, uint16_t size) {
  if (size < 2) {
    sendConfigResponse(ConfigCommand::SetEncoderReversed, ConfigStatus::InvalidLength, nullptr, 0);
    return;
  }

  const bool previous = encoderReversed();
  const bool reversed = buffer[1] != 0;
  setEncoderReversed(reversed);
  if (!saveEncoderReversedToStorage()) {
    setEncoderReversed(previous);
    sendConfigResponse(ConfigCommand::SetEncoderReversed, ConfigStatus::StorageError, nullptr, 0);
    return;
  }

  const uint8_t payload[] = { static_cast<uint8_t>(reversed ? 1 : 0) };
  sendConfigResponse(ConfigCommand::SetEncoderReversed, ConfigStatus::Ok, payload, sizeof(payload));
}

void handleGetKey(const uint8_t* buffer, uint16_t size) {
  if (size < 3) {
    sendConfigResponse(ConfigCommand::GetKey, ConfigStatus::InvalidLength, nullptr, 0);
    return;
  }

  const uint8_t layer = buffer[1];
  const uint8_t keyIndex = buffer[2];
  if (layer >= Config::LAYER_COUNT || keyIndex >= Config::KEY_COUNT) {
    sendConfigResponse(ConfigCommand::GetKey, ConfigStatus::OutOfRange, nullptr, 0);
    return;
  }

  const KeyAssignment& assignment = assignmentFor(layer, keyIndex);

  uint8_t payload[13] = { 0 };
  payload[0] = layer;
  payload[1] = keyIndex;
  payload[2] = static_cast<uint8_t>(assignment.kind);
  payload[3] = assignment.modifier;
  for (uint8_t i = 0; i < Config::KEYBOARD_REPORT_SLOTS; i++) {
    payload[4 + i] = assignment.keycodes[i];
  }
  payload[10] = static_cast<uint8_t>(assignment.consumerUsage & 0xFF);
  payload[11] = static_cast<uint8_t>((assignment.consumerUsage >> 8) & 0xFF);
  payload[12] = assignment.targetLayer;

  sendConfigResponse(ConfigCommand::GetKey, ConfigStatus::Ok, payload, sizeof(payload));
}

void handleSetKey(const uint8_t* buffer, uint16_t size) {
  if (size < 14) {
    sendConfigResponse(ConfigCommand::SetKey, ConfigStatus::InvalidLength, nullptr, 0);
    return;
  }

  const uint8_t layer = buffer[1];
  const uint8_t keyIndex = buffer[2];
  KeyAssignment assignment = blankAssignment();
  assignment.kind = static_cast<AssignmentKind>(buffer[3]);
  assignment.modifier = buffer[4];

  if (assignment.kind != AssignmentKind::None &&
      assignment.kind != AssignmentKind::Keyboard &&
      assignment.kind != AssignmentKind::Consumer &&
      assignment.kind != AssignmentKind::LayerCycle &&
      assignment.kind != AssignmentKind::MomentaryLayer &&
      assignment.kind != AssignmentKind::LayerPrevious) {
    sendConfigResponse(ConfigCommand::SetKey, ConfigStatus::OutOfRange, nullptr, 0);
    return;
  }

  for (uint8_t i = 0; i < Config::KEYBOARD_REPORT_SLOTS; i++) {
    assignment.keycodes[i] = buffer[5 + i];
  }

  assignment.consumerUsage = static_cast<uint16_t>(buffer[11]) |
                             (static_cast<uint16_t>(buffer[12]) << 8);
  assignment.targetLayer = buffer[13];

  if (assignment.kind == AssignmentKind::MomentaryLayer &&
      assignment.targetLayer >= Config::LAYER_COUNT) {
    sendConfigResponse(ConfigCommand::SetKey, ConfigStatus::OutOfRange, nullptr, 0);
    return;
  }

  if (!setAssignment(layer, keyIndex, assignment)) {
    sendConfigResponse(ConfigCommand::SetKey, ConfigStatus::OutOfRange, nullptr, 0);
    return;
  }

  if (!saveAssignmentToStorage(layer, keyIndex)) {
    sendConfigResponse(ConfigCommand::SetKey, ConfigStatus::StorageError, nullptr, 0);
    return;
  }

  const uint8_t payload[] = { layer, keyIndex };
  sendConfigResponse(ConfigCommand::SetKey, ConfigStatus::Ok, payload, sizeof(payload));
}

void handleEnterBootloader() {
#if defined(ARDUINO_ARCH_RP2040)
  sendConfigResponse(ConfigCommand::EnterBootloader, ConfigStatus::Ok, nullptr, 0);
  delay(100);
  rp2040.rebootToBootloader();
#else
  sendConfigResponse(ConfigCommand::EnterBootloader, ConfigStatus::Unsupported, nullptr, 0);
#endif
}

void handleRemapperHeartbeat() {
  if (!remapperConnected()) {
    queueAllKeyboardReleases();
    queueAllConsumerReleases();
  }
  lastRemapperHeartbeatMs = millis();
}

void handleDiagnosticReport(const uint8_t* buffer, uint16_t size) {
  if (size < 5) {
    sendConfigResponse(ConfigCommand::DiagnosticReport, ConfigStatus::InvalidLength, nullptr, 0);
    return;
  }

  const uint8_t payload[] = {
    0x52, 0x50, 0x54, 0x01,
    buffer[1], buffer[2], buffer[3], buffer[4],
  };

  sendConfigResponse(ConfigCommand::DiagnosticReport, ConfigStatus::Ok, payload, sizeof(payload));
}

void handleDiagnosticStorage() {
  const bool ok = runKeymapStorageSelfTest();
  const uint8_t payload[] = {
    static_cast<uint8_t>(ok ? 1 : 0),
    Config::LAYER_COUNT,
    Config::KEY_COUNT,
  };

  sendConfigResponse(ConfigCommand::DiagnosticStorage, ok ? ConfigStatus::Ok : ConfigStatus::StorageError, payload, sizeof(payload));
}

void sendKeyEvent(uint8_t layer, uint8_t keyIndex, bool pressed) {
  if (!remapperConnected()) {
    return;
  }

  const uint8_t payload[] = {
    layer,
    keyIndex,
    static_cast<uint8_t>(pressed ? 1 : 0),
  };

  sendConfigResponse(ConfigCommand::KeyEvent, ConfigStatus::Ok, payload, sizeof(payload));
}

void setReportCallback(uint8_t reportId, hid_report_type_t reportType, uint8_t const* buffer, uint16_t size) {
  if (reportId != RID_CONFIG || (reportType != HID_REPORT_TYPE_OUTPUT && reportType != HID_REPORT_TYPE_FEATURE)) {
    return;
  }

  if (size == 0) {
    return;
  }

  const ConfigCommand command = static_cast<ConfigCommand>(buffer[0]);

  switch (command) {
    case ConfigCommand::GetState:
      handleGetState();
      break;
    case ConfigCommand::SetLayer:
      handleSetLayer(buffer, size);
      break;
    case ConfigCommand::GetKey:
      handleGetKey(buffer, size);
      break;
    case ConfigCommand::SetKey:
      handleSetKey(buffer, size);
      break;
    case ConfigCommand::EnterBootloader:
      handleEnterBootloader();
      break;
    case ConfigCommand::RemapperHeartbeat:
      handleRemapperHeartbeat();
      break;
    case ConfigCommand::KeyEvent:
      sendConfigResponse(command, ConfigStatus::Unsupported, nullptr, 0);
      break;
    case ConfigCommand::DiagnosticReport:
      handleDiagnosticReport(buffer, size);
      break;
    case ConfigCommand::DiagnosticStorage:
      handleDiagnosticStorage();
      break;
    case ConfigCommand::SetLayerEnabled:
      handleSetLayerEnabled(buffer, size);
      break;
    case ConfigCommand::GetLayerColor:
      handleGetLayerColor(buffer, size);
      break;
    case ConfigCommand::SetLayerColor:
      handleSetLayerColor(buffer, size);
      break;
    case ConfigCommand::PreviewLayerColor:
      handlePreviewLayerColor(buffer, size);
      break;
    case ConfigCommand::ResetConfiguration:
      handleResetConfiguration();
      break;
    case ConfigCommand::SetEncoderReversed:
      handleSetEncoderReversed(buffer, size);
      break;
    default:
      sendConfigResponse(command, ConfigStatus::UnknownCommand, nullptr, 0);
      break;
  }
}

bool anyMomentaryLayerActive() {
  for (uint8_t keyIndex = 0; keyIndex < Config::KEY_COUNT; keyIndex++) {
    if (momentaryLayerActive[keyIndex]) {
      return true;
    }
  }

  return false;
}

void applyMomentaryLayerState() {
  uint8_t layer = momentaryBaseLayer;
  uint32_t latestOrder = 0;

  for (uint8_t keyIndex = 0; keyIndex < Config::KEY_COUNT; keyIndex++) {
    if (momentaryLayerActive[keyIndex] && momentaryLayerOrder[keyIndex] >= latestOrder) {
      latestOrder = momentaryLayerOrder[keyIndex];
      layer = momentaryLayerTargets[keyIndex];
    }
  }

  setActiveLayer(layer);
}

void pressMomentaryLayer(uint8_t keyIndex, uint8_t sourceLayer, uint8_t targetLayer) {
  if (!layerEnabled(targetLayer)) {
    return;
  }

  if (!anyMomentaryLayerActive()) {
    momentaryBaseLayer = sourceLayer;
  }

  momentaryLayerActive[keyIndex] = true;
  momentaryLayerTargets[keyIndex] = targetLayer;
  momentaryLayerOrder[keyIndex] = nextMomentaryLayerOrder++;
  queueAllKeyboardReleases();
  applyMomentaryLayerState();
}

bool releaseMomentaryLayer(uint8_t keyIndex) {
  if (!momentaryLayerActive[keyIndex]) {
    return false;
  }

  momentaryLayerActive[keyIndex] = false;
  momentaryLayerOrder[keyIndex] = 0;
  queueAllKeyboardReleases();
  applyMomentaryLayerState();
  return true;
}

uint8_t nextEnabledLayer(uint8_t layer) {
  for (uint8_t offset = 1; offset <= Config::LAYER_COUNT; offset++) {
    const uint8_t candidate = static_cast<uint8_t>((layer + offset) % Config::LAYER_COUNT);
    if (layerEnabled(candidate)) {
      return candidate;
    }
  }

  return 0;
}

uint8_t previousEnabledLayer(uint8_t layer) {
  for (uint8_t offset = 1; offset <= Config::LAYER_COUNT; offset++) {
    const uint8_t candidate = static_cast<uint8_t>((layer + Config::LAYER_COUNT - offset) % Config::LAYER_COUNT);
    if (layerEnabled(candidate)) {
      return candidate;
    }
  }

  return 0;
}

void cyclePersistentLayer() {
  if (anyMomentaryLayerActive()) {
    momentaryBaseLayer = nextEnabledLayer(momentaryBaseLayer);
    applyMomentaryLayerState();
    return;
  }

  setActiveLayer(nextEnabledLayer(activeLayer()));
}

void cyclePersistentLayerBackward() {
  if (anyMomentaryLayerActive()) {
    momentaryBaseLayer = previousEnabledLayer(momentaryBaseLayer);
    applyMomentaryLayerState();
    return;
  }

  setActiveLayer(previousEnabledLayer(activeLayer()));
}

void queueWakeKeyChange(Config::KeyMask oldMask, Config::KeyMask newMask, uint8_t layer) {
  if (!wakeKeyChangePending) {
    wakeOldMask = oldMask;
  }

  wakeNewMask = newMask;
  wakeLayer = layer;
  wakeKeyChangePending = true;
}

void flushWakeKeyChange() {
  if (!wakeKeyChangePending || TinyUSBDevice.suspended()) {
    return;
  }

  const Config::KeyMask oldMask = wakeOldMask;
  const Config::KeyMask newMask = wakeNewMask;
  const uint8_t layer = wakeLayer;
  wakeKeyChangePending = false;
  sendKeyChanges(oldMask, newMask, layer);
}

}  // namespace

void beginHidDevice() {
#if defined(ARDUINO_ARCH_MBED) && defined(ARDUINO_ARCH_RP2040)
  TinyUSB_Device_Init(0);
#endif

  beginReadmeDrive();

  usbHid.setReportCallback(nullptr, setReportCallback);
  usbHid.begin();
}

bool hidDeviceMounted() {
  return TinyUSBDevice.mounted();
}

void updateHidDevice() {
  flushWakeKeyChange();
  serviceConsumerReports();

  if (flushKeyboardReport(usbHid)) {
    return;
  }

  flushConsumerReport(usbHid);
}

bool remapperConnected() {
  if (lastRemapperHeartbeatMs == 0) {
    return false;
  }

  if (millis() - lastRemapperHeartbeatMs > Config::REMAPPER_HEARTBEAT_TIMEOUT_MS) {
    lastRemapperHeartbeatMs = 0;
    return false;
  }

  return true;
}

void sendKeyChanges(Config::KeyMask oldMask, Config::KeyMask newMask, uint8_t layer) {
  if (TinyUSBDevice.suspended()) {
    queueWakeKeyChange(oldMask, newMask, layer);
    TinyUSBDevice.remoteWakeup();
    return;
  }

  const Config::KeyMask changed = oldMask ^ newMask;
  const bool remapperActive = remapperConnected();

  for (uint8_t keyIndex = 0; keyIndex < Config::KEY_COUNT; keyIndex++) {
    const Config::KeyMask bit = static_cast<Config::KeyMask>(1U << keyIndex);
    if ((changed & bit) == 0) {
      continue;
    }

    const bool pressed = (newMask & bit) != 0;
    const KeyAssignment& assignment = assignmentFor(layer, keyIndex);

    if (pressed && remapperActive) {
      sendKeyEvent(layer, keyIndex, pressed);
    }

    if (!pressed) {
      queueKeyboardRelease(keyIndex);
      queueConsumerRelease(keyIndex);
      if (releaseMomentaryLayer(keyIndex)) {
        continue;
      }
      continue;
    }

    if (remapperActive) {
      queueKeyboardRelease(keyIndex);
      queueConsumerRelease(keyIndex);
      continue;
    }

    switch (assignment.kind) {
      case AssignmentKind::Keyboard:
        queueKeyboardAssignment(keyIndex, assignment);
        break;
      case AssignmentKind::Consumer:
        if (keyIndex == Config::ENCODER_CCW_KEY_INDEX || keyIndex == Config::ENCODER_CW_KEY_INDEX) {
          queueConsumerTap(assignment.consumerUsage);
        } else {
          queueConsumerPress(keyIndex, assignment.consumerUsage);
        }
        break;
      case AssignmentKind::LayerCycle:
        queueAllKeyboardReleases();
        cyclePersistentLayer();
        break;
      case AssignmentKind::LayerPrevious:
        queueAllKeyboardReleases();
        cyclePersistentLayerBackward();
        break;
      case AssignmentKind::MomentaryLayer:
        pressMomentaryLayer(keyIndex, layer, assignment.targetLayer);
        break;
      case AssignmentKind::None:
      default:
        queueKeyboardRelease(keyIndex);
        queueConsumerRelease(keyIndex);
        break;
    }
  }
}
