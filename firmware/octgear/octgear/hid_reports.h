#pragma once

#include <Arduino.h>

enum HidReportId : uint8_t {
  RID_KEYBOARD = 1,
  RID_CONSUMER_CONTROL,
  RID_CONFIG,
};

enum class ConfigCommand : uint8_t {
  None = 0x00,
  GetState = 0x01,
  SetLayer = 0x02,
  GetKey = 0x03,
  SetKey = 0x04,
  EnterBootloader = 0x05,
  RemapperHeartbeat = 0x06,
  KeyEvent = 0x07,
  DiagnosticReport = 0x08,
  DiagnosticStorage = 0x09,
  SetLayerEnabled = 0x0A,
  GetLayerColor = 0x0B,
  SetLayerColor = 0x0C,
  PreviewLayerColor = 0x0D,
  ResetConfiguration = 0x0E,
};

enum class ConfigStatus : uint8_t {
  Ok = 0x00,
  InvalidLength = 0x01,
  OutOfRange = 0x02,
  StorageError = 0x03,
  Unsupported = 0x04,
  UnknownCommand = 0xFF,
};
