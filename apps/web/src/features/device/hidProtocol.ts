export const CONFIG_REPORT_ID = 3;
export const CONFIG_REPORT_SIZE = 32;

export enum ConfigCommand {
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
  SetLayerEnabled = 0x0a,
  GetLayerColor = 0x0b,
  SetLayerColor = 0x0c,
  PreviewLayerColor = 0x0d,
  ResetConfiguration = 0x0e,
  SetEncoderReversed = 0x0f,
  SetStatusLedBrightness = 0x10,
}

export enum ConfigStatus {
  Ok = 0x00,
  InvalidLength = 0x01,
  OutOfRange = 0x02,
  StorageError = 0x03,
  Unsupported = 0x04,
  UnknownCommand = 0xff,
}

export type ConfigResponse = {
  command: ConfigCommand;
  status: ConfigStatus;
  payload: Uint8Array;
  raw: Uint8Array;
};

export function createConfigReport(command: ConfigCommand, payload: ArrayLike<number> = []) {
  if (payload.length > CONFIG_REPORT_SIZE - 1) {
    throw new Error(`HID payload is too large: ${payload.length} bytes`);
  }

  const report = new Uint8Array(CONFIG_REPORT_SIZE);
  report[0] = command;

  for (let i = 0; i < payload.length; i++) {
    report[i + 1] = payload[i] & 0xff;
  }

  return report;
}

export function decodeConfigResponse(raw: Uint8Array): ConfigResponse {
  if (raw.length !== CONFIG_REPORT_SIZE) {
    throw new Error(`Invalid HID report length: ${raw.length}`);
  }

  const payloadLength = raw[2];
  const safePayloadLength = Math.min(payloadLength, CONFIG_REPORT_SIZE - 3);

  return {
    command: raw[0] as ConfigCommand,
    status: raw[1] as ConfigStatus,
    payload: raw.slice(3, 3 + safePayloadLength),
    raw,
  };
}

export function assertConfigOk(response: ConfigResponse) {
  if (response.status === ConfigStatus.Ok) {
    return;
  }

  throw new Error(`Device rejected HID command ${response.command}: status ${response.status}`);
}
