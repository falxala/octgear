import { t } from "../../shared/i18n";
import { HARDWARE_CONFIG } from "../hardware/hardwareConfig";

export type KeyAssignmentKind = "none" | "keyboard" | "consumer" | "layerCycle" | "momentaryLayer";

export type KeyAssignment = {
  kind: KeyAssignmentKind;
  modifier: number;
  usage: number;
  targetLayer: number;
  keycodes: number[];
  label: string;
};

export type LayerKeymap = KeyAssignment[];

export type DeviceKeymap = LayerKeymap[];

export function createBlankAssignment(): KeyAssignment {
  return normalizeAssignment({ kind: "none" });
}

export function createKeyboardAssignment(
  usage: number,
  modifier = 0,
  keycodes: number[] = [usage],
): KeyAssignment {
  return normalizeAssignment({ kind: "keyboard", modifier, usage, keycodes });
}

export function createConsumerAssignment(usage: number): KeyAssignment {
  return normalizeAssignment({ kind: "consumer", usage });
}

export function createLayerCycleAssignment(): KeyAssignment {
  return normalizeAssignment({ kind: "layerCycle" });
}

export function createMomentaryLayerAssignment(layer: number): KeyAssignment {
  return normalizeAssignment({ kind: "momentaryLayer", usage: layer });
}

export function normalizeAssignment(
  assignment: Partial<KeyAssignment> & { kind: KeyAssignmentKind },
): KeyAssignment {
  const kind = assignment.kind;

  if (kind === "none") {
    return {
      kind,
      modifier: 0,
      usage: 0,
      targetLayer: 0,
      keycodes: createEmptyKeycodes(),
      label: t.keymap.unassigned,
    };
  }

  if (kind === "consumer") {
    const usage = clampBytePair(assignment.usage ?? 0);

    return {
      kind,
      modifier: 0,
      usage,
      targetLayer: 0,
      keycodes: createEmptyKeycodes(),
      label: consumerLabels[usage] ?? `Consumer ${formatHex(usage, 4)}`,
    };
  }

  if (kind === "layerCycle") {
    return {
      kind,
      modifier: 0,
      usage: 0,
      targetLayer: 0,
      keycodes: createEmptyKeycodes(),
      label: t.assignment.layerCycleLabel,
    };
  }

  if (kind === "momentaryLayer") {
    const targetLayer = clampLayer(assignment.targetLayer ?? assignment.usage ?? 0);

    return {
      kind,
      modifier: 0,
      usage: targetLayer,
      targetLayer,
      keycodes: createEmptyKeycodes(),
      label: t.assignment.momentaryLayerLabel(targetLayer),
    };
  }

  const keycodes = normalizeKeycodes(assignment.keycodes ?? [assignment.usage ?? 0]);
  const usage = keycodes[0] ?? 0;
  const modifier = clampByte(assignment.modifier ?? 0);
  const prefix = formatModifier(modifier);
  const label = keyboardLabels[usage] ?? `Key ${formatHex(usage)}`;
  const displayLabel = prefix && usage === 0 ? prefix : label;

  return {
    kind,
    modifier,
    usage,
    targetLayer: 0,
    keycodes,
    label: prefix && usage !== 0 ? `${prefix}+${displayLabel}` : displayLabel,
  };
}

export function formatHex(value: number, width = 2) {
  return `0x${value.toString(16).toUpperCase().padStart(width, "0")}`;
}

export function sameAssignment(first: KeyAssignment, second: KeyAssignment) {
  return (
    first.kind === second.kind &&
    first.modifier === second.modifier &&
    first.usage === second.usage &&
    first.targetLayer === second.targetLayer &&
    first.keycodes.every((keycode, index) => keycode === second.keycodes[index])
  );
}

function createEmptyKeycodes() {
  return [0, 0, 0, 0, 0, 0];
}

function normalizeKeycodes(keycodes: number[]) {
  const next = createEmptyKeycodes();

  for (let i = 0; i < next.length && i < keycodes.length; i++) {
    next[i] = clampByte(keycodes[i] ?? 0);
  }

  return next;
}

function clampByte(value: number) {
  return Math.max(0, Math.min(0xff, Math.trunc(value)));
}

function clampBytePair(value: number) {
  return Math.max(0, Math.min(0xffff, Math.trunc(value)));
}

function clampLayer(value: number) {
  return Math.max(0, Math.min(HARDWARE_CONFIG.layerCount - 1, Math.trunc(value)));
}

export function formatModifier(modifier: number) {
  const names = [
    [0x01, "Ctrl"],
    [0x02, "Shift"],
    [0x04, "Alt"],
    [0x08, "Meta"],
    [0x10, "RCtrl"],
    [0x20, "RShift"],
    [0x40, "RAlt"],
    [0x80, "RMeta"],
  ] as const;

  return names
    .filter(([bit]) => (modifier & bit) !== 0)
    .map(([, name]) => name)
    .join("+");
}

const keyboardLabels: Record<number, string> = {
  0x00: "None",
  0x04: "A",
  0x05: "B",
  0x06: "C",
  0x07: "D",
  0x08: "E",
  0x09: "F",
  0x0a: "G",
  0x0b: "H",
  0x0c: "I",
  0x0d: "J",
  0x0e: "K",
  0x0f: "L",
  0x10: "M",
  0x11: "N",
  0x12: "O",
  0x13: "P",
  0x14: "Q",
  0x15: "R",
  0x16: "S",
  0x17: "T",
  0x18: "U",
  0x19: "V",
  0x1a: "W",
  0x1b: "X",
  0x1c: "Y",
  0x1d: "Z",
  0x1e: "1",
  0x1f: "2",
  0x20: "3",
  0x21: "4",
  0x22: "5",
  0x23: "6",
  0x24: "7",
  0x25: "8",
  0x26: "9",
  0x27: "0",
  0x28: "Enter",
  0x29: "Esc",
  0x2a: "Back",
  0x2b: "Tab",
  0x2c: "Space",
  0x2d: "-",
  0x2e: "^",
  0x2f: "@",
  0x30: "[",
  0x32: "]",
  0x33: ";",
  0x34: ":",
  0x35: "半角/",
  0x36: ",",
  0x37: ".",
  0x38: "/",
  0x39: "Caps",
  0x3a: "F1",
  0x3b: "F2",
  0x3c: "F3",
  0x3d: "F4",
  0x3e: "F5",
  0x3f: "F6",
  0x40: "F7",
  0x41: "F8",
  0x42: "F9",
  0x43: "F10",
  0x44: "F11",
  0x45: "F12",
  0x46: "PrtSc",
  0x47: "ScrLk",
  0x48: "Pause",
  0x49: "Insert",
  0x4a: "Home",
  0x4b: "Page Up",
  0x4c: "Delete",
  0x4d: "End",
  0x4e: "Page Down",
  0x4f: "Right",
  0x50: "Left",
  0x51: "Down",
  0x52: "Up",
  0x53: "Num",
  0x54: "Pad/",
  0x55: "Pad*",
  0x56: "Pad-",
  0x57: "Pad+",
  0x58: "Pad Enter",
  0x59: "Pad1",
  0x5a: "Pad2",
  0x5b: "Pad3",
  0x5c: "Pad4",
  0x5d: "Pad5",
  0x5e: "Pad6",
  0x5f: "Pad7",
  0x60: "Pad8",
  0x61: "Pad9",
  0x62: "Pad0",
  0x63: "Pad.",
  0x65: "Menu",
  0x87: "_",
  0x88: "カナ",
  0x89: "Yen",
  0x8a: "変換",
  0x8b: "無変換",
};

const consumerLabels: Record<number, string> = {
  0x006f: "Brightness +",
  0x0070: "Brightness -",
  0x00b5: "Next",
  0x00b6: "Previous",
  0x00cd: "Play/Pause",
  0x00e2: "Mute",
  0x00e9: "Volume +",
  0x00ea: "Volume -",
};
