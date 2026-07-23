import type { KeyAssignment } from "../keymap/keymapTypes";
import {
  createBlankAssignment,
  createConsumerAssignment,
  createKeyboardAssignment,
  createLayerCycleAssignment,
  createLayerPreviousAssignment,
  createMomentaryLayerAssignment,
  normalizeAssignment,
} from "../keymap/keymapTypes";
import {
  assertConfigOk,
  ConfigCommand,
  ConfigStatus,
  createConfigReport,
  decodeConfigResponse,
  type ConfigResponse,
} from "./hidProtocol";
import type { WebHidTransport } from "./webHidTransport";
import { t } from "../../shared/i18n";

export type DeviceState = {
  activeLayer: number;
  layerCount: number;
  keyCount: number;
  matrixRowCount: number;
  encoderReversed: boolean;
  statusLedBrightness: number;
  statusLedBrightnessSupported: boolean;
  enabledLayers: boolean[];
};

export type LayerEnabledResult = {
  activeLayer: number;
  enabledLayers: boolean[];
};

export type LayerColor = {
  red: number;
  green: number;
  blue: number;
};

export type DeviceKeyEvent = {
  layer: number;
  keyIndex: number;
  pressed: boolean;
};

export type DiagnosticReportResult = {
  signature: string;
  version: number;
};

export type DiagnosticStorageResult = {
  layerCount: number;
  keyCount: number;
};

const DIAGNOSTIC_REPORT_NONCE = [0x43, 0x59, 0x42, 0x38] as const;

export async function getDeviceState(transport: WebHidTransport): Promise<DeviceState> {
  const response = await sendCommand(transport, ConfigCommand.GetState);
  assertConfigOk(response);

  const layerCount = response.payload[1] ?? 0;
  const enabledLayerMask = response.payload[4] ?? ((1 << layerCount) - 1);

  return {
    activeLayer: response.payload[0] ?? 0,
    layerCount,
    keyCount: response.payload[2] ?? 0,
    matrixRowCount: response.payload[3] ?? 0,
    encoderReversed: (response.payload[5] ?? 0) !== 0,
    statusLedBrightness: response.payload[6] ?? 0,
    statusLedBrightnessSupported: response.payload.length >= 7,
    enabledLayers: decodeEnabledLayers(enabledLayerMask, layerCount),
  };
}

export async function setDeviceLayer(transport: WebHidTransport, layer: number) {
  const response = await sendCommand(transport, ConfigCommand.SetLayer, [layer]);
  assertConfigOk(response);
}

export async function setDeviceEncoderReversed(transport: WebHidTransport, reversed: boolean) {
  const response = await sendCommand(transport, ConfigCommand.SetEncoderReversed, [reversed ? 1 : 0]);
  if (response.status === ConfigStatus.UnknownCommand || response.status === ConfigStatus.Unsupported) {
    throw new Error(t.device.encoderReverseUnsupported);
  }
  assertConfigOk(response);
  return (response.payload[0] ?? 0) !== 0;
}

export async function setDeviceStatusLedBrightness(transport: WebHidTransport, brightness: number) {
  const response = await sendCommand(transport, ConfigCommand.SetStatusLedBrightness, [brightness]);
  if (response.status === ConfigStatus.UnknownCommand || response.status === ConfigStatus.Unsupported) {
    throw new Error(t.device.statusLedBrightnessUnsupported);
  }
  assertConfigOk(response);
  return response.payload[0] ?? brightness;
}

export async function setDeviceLayerEnabled(
  transport: WebHidTransport,
  layer: number,
  enabled: boolean,
  layerCount: number,
): Promise<LayerEnabledResult> {
  const response = await sendCommand(transport, ConfigCommand.SetLayerEnabled, [layer, enabled ? 1 : 0]);
  if (response.status === ConfigStatus.UnknownCommand || response.status === ConfigStatus.Unsupported) {
    throw new Error(t.device.layerEnabledUnsupported);
  }
  assertConfigOk(response);

  return {
    activeLayer: response.payload[2] ?? 0,
    enabledLayers: decodeEnabledLayers(response.payload[3] ?? 1, layerCount),
  };
}

export async function readDeviceLayerColors(
  transport: WebHidTransport,
  layerCount: number,
  fallbackColors: readonly (readonly [number, number, number])[],
): Promise<LayerColor[]> {
  const colors: LayerColor[] = [];

  for (let layer = 0; layer < layerCount; layer++) {
    const response = await sendCommand(transport, ConfigCommand.GetLayerColor, [layer]);
    if (response.status === ConfigStatus.UnknownCommand || response.status === ConfigStatus.Unsupported) {
      return Array.from({ length: layerCount }, (_, index) => colorFromTuple(fallbackColors[index]));
    }
    assertConfigOk(response);
    colors.push({
      red: response.payload[1] ?? 0,
      green: response.payload[2] ?? 0,
      blue: response.payload[3] ?? 0,
    });
  }

  return colors;
}

export async function setDeviceLayerColor(
  transport: WebHidTransport,
  layer: number,
  color: LayerColor,
) {
  const response = await sendCommand(transport, ConfigCommand.SetLayerColor, [
    layer,
    clampColorChannel(color.red),
    clampColorChannel(color.green),
    clampColorChannel(color.blue),
  ]);
  if (response.status === ConfigStatus.UnknownCommand || response.status === ConfigStatus.Unsupported) {
    throw new Error(t.device.layerColorUnsupported);
  }
  assertConfigOk(response);
}

export async function previewDeviceLayerColor(
  transport: WebHidTransport,
  layer: number,
  color: LayerColor,
) {
  const response = await sendCommand(transport, ConfigCommand.PreviewLayerColor, [
    layer,
    clampColorChannel(color.red),
    clampColorChannel(color.green),
    clampColorChannel(color.blue),
  ]);
  assertConfigOk(response);
}

export async function clearDeviceLayerColorPreview(transport: WebHidTransport) {
  const response = await sendCommand(transport, ConfigCommand.PreviewLayerColor);
  assertConfigOk(response);
}

export async function resetDeviceConfiguration(transport: WebHidTransport) {
  const response = await sendCommand(transport, ConfigCommand.ResetConfiguration);
  assertConfigOk(response);
}

export async function getDeviceKey(transport: WebHidTransport, layer: number, keyIndex: number) {
  const response = await sendCommand(transport, ConfigCommand.GetKey, [layer, keyIndex]);
  assertConfigOk(response);
  return decodeAssignmentPayload(response.payload);
}

export async function readDeviceKeymap(
  transport: WebHidTransport,
  layerCount: number,
  keyCount: number,
) {
  const keymap: KeyAssignment[][] = [];

  for (let layer = 0; layer < layerCount; layer++) {
    const layerAssignments: KeyAssignment[] = [];

    for (let keyIndex = 0; keyIndex < keyCount; keyIndex++) {
      layerAssignments.push(await getDeviceKey(transport, layer, keyIndex));
    }

    keymap.push(layerAssignments);
  }

  return keymap;
}

export async function setDeviceKey(
  transport: WebHidTransport,
  layer: number,
  keyIndex: number,
  assignment: KeyAssignment,
) {
  const normalized = normalizeAssignment(assignment);
  const payload = new Uint8Array(13);
  payload[0] = layer;
  payload[1] = keyIndex;
  payload[2] = encodeAssignmentKind(normalized.kind);
  payload[3] = normalized.modifier;

  if (normalized.kind === "keyboard") {
    for (let i = 0; i < 6; i++) {
      payload[4 + i] = normalized.keycodes[i] ?? 0;
    }
  }

  if (normalized.kind === "consumer") {
    payload[10] = normalized.usage & 0xff;
    payload[11] = (normalized.usage >> 8) & 0xff;
  }

  if (normalized.kind === "momentaryLayer") {
    payload[12] = normalized.targetLayer & 0xff;
  }

  const response = await sendCommand(transport, ConfigCommand.SetKey, payload);
  assertConfigOk(response);
}

export async function enterDeviceBootloader(transport: WebHidTransport) {
  await transport.sendConfigReport(createConfigReport(ConfigCommand.EnterBootloader));
}

export async function sendRemapperHeartbeat(transport: WebHidTransport) {
  await transport.sendConfigReport(createConfigReport(ConfigCommand.RemapperHeartbeat));
}

export async function runDiagnosticReportTest(transport: WebHidTransport): Promise<DiagnosticReportResult> {
  const response = await sendCommand(transport, ConfigCommand.DiagnosticReport, DIAGNOSTIC_REPORT_NONCE);
  if (response.status === ConfigStatus.UnknownCommand || response.status === ConfigStatus.Unsupported) {
    throw new Error(t.device.diagnosticReportUnsupported);
  }

  assertConfigOk(response);

  const expected = [0x52, 0x50, 0x54, 0x01, ...DIAGNOSTIC_REPORT_NONCE];
  for (let i = 0; i < expected.length; i++) {
    if (response.payload[i] !== expected[i]) {
      throw new Error(t.device.invalidDiagnosticReport);
    }
  }

  return {
    signature: "RPT",
    version: response.payload[3] ?? 0,
  };
}

export async function runDiagnosticStorageTest(transport: WebHidTransport): Promise<DiagnosticStorageResult> {
  const response = await sendCommand(transport, ConfigCommand.DiagnosticStorage);
  if (response.status === ConfigStatus.UnknownCommand || response.status === ConfigStatus.Unsupported) {
    throw new Error(t.device.diagnosticStorageUnsupported);
  }

  assertConfigOk(response);

  return {
    layerCount: response.payload[1] ?? 0,
    keyCount: response.payload[2] ?? 0,
  };
}

export function subscribeDeviceKeyEvents(
  transport: WebHidTransport,
  handler: (event: DeviceKeyEvent) => void,
) {
  return transport.addConfigReportListener((raw) => {
    const response = decodeConfigResponse(raw);
    if (response.command !== ConfigCommand.KeyEvent) {
      return;
    }

    assertConfigOk(response);
    handler({
      layer: response.payload[0] ?? 0,
      keyIndex: response.payload[1] ?? 0,
      pressed: (response.payload[2] ?? 0) !== 0,
    });
  });
}

async function sendCommand(
  transport: WebHidTransport,
  command: ConfigCommand,
  payload: ArrayLike<number> = [],
): Promise<ConfigResponse> {
  const raw = await transport.requestConfigReport(createConfigReport(command, payload));
  const response = decodeConfigResponse(raw);

  if (response.command !== command) {
    throw new Error(t.device.unexpectedResponse(response.command, command));
  }

  return response;
}

function encodeAssignmentKind(kind: KeyAssignment["kind"]) {
  switch (kind) {
    case "keyboard":
      return 1;
    case "consumer":
      return 2;
    case "layerCycle":
      return 3;
    case "layerPrevious":
      return 5;
    case "momentaryLayer":
      return 4;
    case "none":
    default:
      return 0;
  }
}

function decodeAssignmentPayload(payload: Uint8Array): KeyAssignment {
  const kind = payload[2] ?? 0;
  const modifier = payload[3] ?? 0;
  const keycodes = Array.from(payload.slice(4, 10));
  const consumerUsage = (payload[10] ?? 0) | ((payload[11] ?? 0) << 8);
  const targetLayer = payload[12] ?? 0;

  if (kind === 1) {
    return createKeyboardAssignment(keycodes[0] ?? 0, modifier, keycodes);
  }

  if (kind === 2) {
    return createConsumerAssignment(consumerUsage);
  }

  if (kind === 3) {
    return createLayerCycleAssignment();
  }

  if (kind === 4) {
    return createMomentaryLayerAssignment(targetLayer);
  }

  if (kind === 5) {
    return createLayerPreviousAssignment();
  }

  return createBlankAssignment();
}

function decodeEnabledLayers(mask: number, layerCount: number) {
  return Array.from({ length: layerCount }, (_, layer) => (mask & (1 << layer)) !== 0);
}

function colorFromTuple(color: readonly [number, number, number] | undefined): LayerColor {
  return {
    red: color?.[0] ?? 0,
    green: color?.[1] ?? 0,
    blue: color?.[2] ?? 0,
  };
}

function clampColorChannel(value: number) {
  return Math.max(0, Math.min(255, Math.trunc(value)));
}
