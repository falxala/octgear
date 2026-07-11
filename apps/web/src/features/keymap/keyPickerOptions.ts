import { t } from "../../shared/i18n";

export type KeyboardLayoutMode = "jis" | "us";

export type KeyboardKeyOption = {
  kind: "key";
  code: number;
  label: string;
  jisLabel?: string;
  usLabel?: string;
  accent?: boolean;
  width?: number;
};

export type ModifierKeyOption = {
  kind: "modifier";
  modifier: number;
  label: string;
  accent?: boolean;
  width?: number;
};

export type BlankKeyOption = {
  kind: "blank";
  label: string;
  accent?: boolean;
  width?: number;
};

export type SpacerOption = {
  kind: "spacer";
  width: number;
};

export type DecorationOption = {
  kind: "decoration";
  decoration: "jis-enter-lower" | "numpad-enter-lower" | "numpad-plus-lower";
  sourceCode: number;
  width: number;
};

export type KeyPickerOption =
  | KeyboardKeyOption
  | ModifierKeyOption
  | BlankKeyOption
  | SpacerOption
  | DecorationOption;

export type ConsumerKeyOption = {
  usage: number;
  label: string;
  icon:
    | "mute"
    | "volume-down"
    | "volume-up"
    | "previous"
    | "play-pause"
    | "next"
    | "brightness-down"
    | "brightness-up";
};

const functionRow: KeyPickerOption[] = [
  key(41, "Esc"),
  gap(0.5),
  key(58, "F1"),
  key(59, "F2"),
  key(60, "F3"),
  key(61, "F4"),
  gap(0.5),
  key(62, "F5"),
  key(63, "F6"),
  key(64, "F7"),
  key(65, "F8"),
  gap(0.5),
  key(66, "F9"),
  key(67, "F10"),
  key(68, "F11"),
  key(69, "F12"),
];

const jisKeyboardRows: KeyPickerOption[][] = [
  functionRow,
  [
    key(53, "E/J", { jisLabel: "E/J", usLabel: "~", accent: true }),
    key(30, "1!"),
    key(31, "2", { jisLabel: '2"', usLabel: "2@", accent: true }),
    key(32, "3#"),
    key(33, "4$"),
    key(34, "5%"),
    key(35, "6", { jisLabel: "6&", usLabel: "6^", accent: true }),
    key(36, "7", { jisLabel: "7'", usLabel: "7&", accent: true }),
    key(37, "8", { jisLabel: "8(", usLabel: "8*", accent: true }),
    key(38, "9", { jisLabel: "9)", usLabel: "9(", accent: true }),
    key(39, "0", { jisLabel: "0", usLabel: "0)", accent: true }),
    key(45, "-", { jisLabel: "-=", usLabel: "-_", accent: true }),
    key(46, "=", { jisLabel: "^~", usLabel: "=+", accent: true }),
    key(137, "Yen", { jisLabel: "Yen|", usLabel: "Yen|", accent: true }),
    key(42, "Back", { usLabel: "←", width: 1.5, accent: true }),
  ],
  [
    key(43, "Tab", { width: 1.5 }),
    key(20, "Q"),
    key(26, "W"),
    key(8, "E"),
    key(21, "R"),
    key(23, "T"),
    key(28, "Y"),
    key(24, "U"),
    key(12, "I"),
    key(18, "O"),
    key(19, "P"),
    key(47, "[", { jisLabel: "@`", usLabel: "[{", accent: true }),
    key(48, "]", { jisLabel: "[{", usLabel: "]}", accent: true }),
    key(40, "Enter", { width: 1.75, accent: true }),
  ],
  [
    key(57, "Caps", { width: 1.75 }),
    key(4, "A"),
    key(22, "S"),
    key(7, "D"),
    key(9, "F"),
    key(10, "G"),
    key(11, "H"),
    key(13, "J"),
    key(14, "K"),
    key(15, "L"),
    key(51, ";+"),
    key(52, ":", { jisLabel: ":*", usLabel: "'\"", accent: true }),
    key(50, "]", { jisLabel: "]}", usLabel: "\\|", accent: true }),
    decor("jis-enter-lower", 40, 1.5),
  ],
  [
    mod(0x02, "Shift", 2.25),
    key(29, "Z"),
    key(27, "X"),
    key(6, "C"),
    key(25, "V"),
    key(5, "B"),
    key(17, "N"),
    key(16, "M"),
    key(54, ","),
    key(55, "."),
    key(56, "/?"),
    key(135, "_", { jisLabel: "\\_", usLabel: "\\_", accent: true }),
    mod(0x20, "Shift", 2.25, true),
  ],
  [
    mod(0x01, "Ctrl", 1.25),
    mod(0x08, "Meta", 1.25),
    mod(0x04, "Alt", 1.25),
    key(139, "無変換", { jisLabel: "無変換", width: 1.25, accent: true }),
    key(44, "Space", { width: 5, accent: true }),
    key(138, "変換", { jisLabel: "変換", width: 1.25, accent: true }),
    key(136, "Kana", { jisLabel: "Kana", width: 1.25, accent: true }),
    mod(0x40, "Alt", 1.25, true),
    key(101, "Menu", { width: 1.25 }),
    mod(0x10, "Ctrl", 1.25),
  ],
];

const usKeyboardRows: KeyPickerOption[][] = [
  functionRow,
  [
    key(53, "`~", { accent: true }),
    key(30, "1!"),
    key(31, "2@", { accent: true }),
    key(32, "3#"),
    key(33, "4$"),
    key(34, "5%"),
    key(35, "6^", { accent: true }),
    key(36, "7&", { accent: true }),
    key(37, "8*", { accent: true }),
    key(38, "9(", { accent: true }),
    key(39, "0)", { accent: true }),
    key(45, "-_", { accent: true }),
    key(46, "=+", { accent: true }),
    key(42, "←", { width: 2, accent: true }),
  ],
  [
    key(43, "Tab", { width: 1.5 }),
    key(20, "Q"),
    key(26, "W"),
    key(8, "E"),
    key(21, "R"),
    key(23, "T"),
    key(28, "Y"),
    key(24, "U"),
    key(12, "I"),
    key(18, "O"),
    key(19, "P"),
    key(47, "[{", { accent: true }),
    key(48, "]}", { accent: true }),
    key(49, "\\|", { width: 1.5, accent: true }),
  ],
  [
    key(57, "Caps", { width: 1.75 }),
    key(4, "A"),
    key(22, "S"),
    key(7, "D"),
    key(9, "F"),
    key(10, "G"),
    key(11, "H"),
    key(13, "J"),
    key(14, "K"),
    key(15, "L"),
    key(51, ";:", { accent: true }),
    key(52, "'\"", { accent: true }),
    key(40, "Enter", { width: 2.25, accent: true }),
  ],
  [
    mod(0x02, "Shift", 2.25),
    key(29, "Z"),
    key(27, "X"),
    key(6, "C"),
    key(25, "V"),
    key(5, "B"),
    key(17, "N"),
    key(16, "M"),
    key(54, ",<", { accent: true }),
    key(55, ".>", { accent: true }),
    key(56, "/?"),
    mod(0x20, "Shift", 2.75, true),
  ],
  [
    mod(0x01, "Ctrl", 1.25),
    mod(0x08, "Meta", 1.25),
    mod(0x04, "Alt", 1.25),
    key(44, "Space", { width: 6.25, accent: true }),
    mod(0x40, "Alt", 1.25, true),
    mod(0x80, "Meta", 1.25, true),
    key(101, "Menu", { width: 1.25 }),
    mod(0x10, "Ctrl", 1.25),
  ],
];

export function keyboardRowsForLayout(layout: KeyboardLayoutMode) {
  return layout === "jis" ? jisKeyboardRows : usKeyboardRows;
}

export const navigationRows: KeyPickerOption[][] = [
  [key(70, "PrtSc"), key(71, "ScrLk"), key(72, "Pause")],
  [key(73, "Insert"), key(74, "Home"), key(75, "PgUp")],
  [key(76, "Delete"), key(77, "End"), key(78, "PgDn")],
  [gap(1), key(82, "Up"), gap(1)],
  [key(80, "Left"), key(81, "Down"), key(79, "Right")],
];

export const numpadRows: KeyPickerOption[][] = [
  [key(83, "Num"), key(84, "/"), key(85, "*"), key(86, "-")],
  [key(95, "7"), key(96, "8"), key(97, "9"), key(87, "+")],
  [key(92, "4"), key(93, "5"), key(94, "6"), decor("numpad-plus-lower", 87, 1)],
  [key(89, "1"), key(90, "2"), key(91, "3"), key(88, "Enter")],
  [key(98, "0", { width: 2 }), key(99, "."), decor("numpad-enter-lower", 88, 1)],
];

export const consumerOptions: ConsumerKeyOption[] = [
  { usage: 0x00e2, label: "Mute", icon: "mute" },
  { usage: 0x00ea, label: "Vol-", icon: "volume-down" },
  { usage: 0x00e9, label: "Vol+", icon: "volume-up" },
  { usage: 0x00b6, label: "Prev", icon: "previous" },
  { usage: 0x00cd, label: "Play", icon: "play-pause" },
  { usage: 0x00b5, label: "Next", icon: "next" },
  { usage: 0x0070, label: "Bri-", icon: "brightness-down" },
  { usage: 0x006f, label: "Bri+", icon: "brightness-up" },
];

export function consumerOptionByUsage(usage: number) {
  return consumerOptions.find((option) => option.usage === usage);
}

export const modifierOptions: ModifierKeyOption[] = [
  mod(0x01, "Ctrl"),
  mod(0x02, "Shift"),
  mod(0x04, "Alt"),
  mod(0x08, "Meta"),
  mod(0x10, "RCtrl"),
  mod(0x20, "RShift"),
  mod(0x40, "RAlt"),
  mod(0x80, "RMeta"),
];

export const blankOption: BlankKeyOption = {
  kind: "blank",
  label: t.keymap.unassigned,
  accent: true,
  width: 2,
};

export function keyOptionLabel(option: KeyPickerOption, layout: KeyboardLayoutMode) {
  if (option.kind === "spacer") {
    return "";
  }

  if (option.kind === "decoration") {
    return "";
  }

  if (option.kind !== "key") {
    return option.label;
  }

  return layout === "jis"
    ? option.jisLabel ?? option.label
    : option.usLabel ?? option.label;
}

function key(
  code: number,
  label: string,
  options: Partial<Omit<KeyboardKeyOption, "kind" | "code" | "label">> = {},
): KeyboardKeyOption {
  return { kind: "key", code, label, ...options };
}

function mod(modifier: number, label: string, width = 1, accent = false): ModifierKeyOption {
  return { kind: "modifier", modifier, label, width, accent };
}

function gap(width: number): SpacerOption {
  return { kind: "spacer", width };
}

function decor(decoration: DecorationOption["decoration"], sourceCode: number, width: number): DecorationOption {
  return { kind: "decoration", decoration, sourceCode, width };
}
