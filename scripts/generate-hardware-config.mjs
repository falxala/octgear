#!/usr/bin/env node
import { readFileSync, writeFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const rootDir = join(dirname(fileURLToPath(import.meta.url)), "..");
const profilePath = join(rootDir, "hardware/octgear/profile.json");
const firmwareOut = join(rootDir, "firmware/octgear/octgear/generated_hardware_config.h");
const webOut = join(rootDir, "apps/web/src/features/hardware/generatedHardwareConfig.ts");
const pinoutOut = join(rootDir, "hardware/octgear/pinout.md");

const profile = JSON.parse(readFileSync(profilePath, "utf8"));

validateProfile(profile);

const physicalKeyCount = profile.keys.length;
const encoderControlCount = profile.encoder.enabled ? profile.encoder.controls.length : 0;
const keyCount = physicalKeyCount + encoderControlCount;
const matrixRowPins = profile.matrix.rows.map((row) => row.gpio);
const matrixColumnPins = profile.matrix.columns.map((column) => column.gpio);
const keyMatrixRows = profile.keys.map((key) => key.row);
const keyMatrixColumns = profile.keys.map((key) => key.column);
const encoderControlById = Object.fromEntries(profile.encoder.controls.map((control) => [control.id, control]));
const defaultEnabledLayerMask = profile.defaultEnabledLayers.reduce(
  (mask, layer) => mask | (1 << layer),
  0,
);

writeFileSync(firmwareOut, firmwareHeader(), "utf8");
writeFileSync(webOut, webConfig(), "utf8");
writeFileSync(pinoutOut, pinoutMarkdown(), "utf8");

function validateProfile(value) {
  requireInteger(value.layerCount, "layerCount");
  if (value.layerCount < 1 || value.layerCount > 8) {
    throw new Error("layerCount must be within 1-8 for the layer mask");
  }
  requireArray(value.defaultEnabledLayers, "defaultEnabledLayers");
  requireArray(value.defaultLayerColors, "defaultLayerColors");
  requireArray(value.keys, "keys");
  requireObject(value.matrix, "matrix");
  requireArray(value.matrix.rows, "matrix.rows");
  requireArray(value.matrix.columns, "matrix.columns");
  requireObject(value.encoder, "encoder");

  if (value.matrix.diodeDirection !== "none") {
    throw new Error("matrix.diodeDirection must be none");
  }
  if (value.matrix.rows.length === 0 || value.matrix.columns.length === 0) {
    throw new Error("matrix must contain at least one row and column");
  }

  value.matrix.rows.forEach((row, index) => validateMatrixLine(row, index, `matrix.rows[${index}]`));
  value.matrix.columns.forEach((column, index) => validateMatrixLine(column, index, `matrix.columns[${index}]`));

  const enabledLayerSet = new Set();
  value.defaultEnabledLayers.forEach((layer, index) => {
    requireInteger(layer, `defaultEnabledLayers[${index}]`);
    if (layer < 0 || layer >= value.layerCount) {
      throw new Error(`defaultEnabledLayers[${index}] must be within the layer range`);
    }
    if (enabledLayerSet.has(layer)) {
      throw new Error(`defaultEnabledLayers[${index}] must be unique`);
    }
    enabledLayerSet.add(layer);
  });
  if (!enabledLayerSet.has(0)) {
    throw new Error("defaultEnabledLayers must include Layer 0");
  }

  if (value.defaultLayerColors.length !== value.layerCount) {
    throw new Error("defaultLayerColors length must match layerCount");
  }
  value.defaultLayerColors.forEach((color, layer) => {
    requireArray(color, `defaultLayerColors[${layer}]`);
    if (color.length !== 3) {
      throw new Error(`defaultLayerColors[${layer}] must contain RGB values`);
    }
    color.forEach((channel, index) => {
      requireInteger(channel, `defaultLayerColors[${layer}][${index}]`);
      if (channel < 0 || channel > 255) {
        throw new Error(`defaultLayerColors[${layer}][${index}] must be within 0-255`);
      }
    });
  });

  const keyPositions = new Set();
  value.keys.forEach((key, index) => {
    requireInteger(key.firmwareIndex, `keys[${index}].firmwareIndex`);
    requireInteger(key.row, `keys[${index}].row`);
    requireInteger(key.column, `keys[${index}].column`);
    if (key.firmwareIndex !== index) {
      throw new Error(`keys[${index}].firmwareIndex must be ${index}`);
    }
    if (key.row < 0 || key.row >= value.matrix.rows.length) {
      throw new Error(`keys[${index}].row must reference matrix.rows`);
    }
    if (key.column < 0 || key.column >= value.matrix.columns.length) {
      throw new Error(`keys[${index}].column must reference matrix.columns`);
    }
    const position = `${key.row}:${key.column}`;
    if (keyPositions.has(position)) {
      throw new Error(`keys[${index}] duplicates matrix position ${position}`);
    }
    keyPositions.add(position);
  });

  if (value.encoder.enabled) {
    requireInteger(value.encoder.pinCount, "encoder.pinCount");
    if (value.encoder.pinCount !== 4) {
      throw new Error("encoder.pinCount must be 4 for A/Common/B/SW");
    }
    requireInteger(value.encoder.aPin, "encoder.aPin");
    requireInteger(value.encoder.commonPin, "encoder.commonPin");
    requireInteger(value.encoder.bPin, "encoder.bPin");
    requireInteger(value.encoder.switchPin, "encoder.switchPin");
    requireInteger(value.encoder.stepsPerDetent, "encoder.stepsPerDetent");
    requireArray(value.encoder.controls, "encoder.controls");
    const expectedIds = ["enc-ccw", "enc-cw", "enc-sw"];
    value.encoder.controls.forEach((control, index) => {
      requireInteger(control.firmwareIndex, `encoder.controls[${index}].firmwareIndex`);
      if (control.id !== expectedIds[index]) {
        throw new Error(`encoder.controls[${index}].id must be ${expectedIds[index]}`);
      }
      const expectedIndex = value.keys.length + index;
      if (control.firmwareIndex !== expectedIndex) {
        throw new Error(`encoder.controls[${index}].firmwareIndex must be ${expectedIndex}`);
      }
    });
  }

  const usedPins = new Map();
  value.matrix.rows.forEach((row, index) => registerPin(usedPins, row.gpio, `matrix.rows[${index}]`));
  value.matrix.columns.forEach((column, index) => registerPin(usedPins, column.gpio, `matrix.columns[${index}]`));
  if (value.encoder.enabled) {
    registerPin(usedPins, value.encoder.aPin, "encoder.aPin");
    registerPin(usedPins, value.encoder.commonPin, "encoder.commonPin");
    registerPin(usedPins, value.encoder.bPin, "encoder.bPin");
    registerPin(usedPins, value.encoder.switchPin, "encoder.switchPin");
  }
}

function validateMatrixLine(line, index, name) {
  requireInteger(line.firmwareIndex, `${name}.firmwareIndex`);
  requireInteger(line.gpio, `${name}.gpio`);
  if (line.firmwareIndex !== index) {
    throw new Error(`${name}.firmwareIndex must be ${index}`);
  }
}

function registerPin(usedPins, pin, name) {
  if (usedPins.has(pin)) {
    throw new Error(`${name} conflicts with ${usedPins.get(pin)} on GPIO ${pin}`);
  }
  usedPins.set(pin, name);
}

function firmwareHeader() {
  return `// Generated from hardware/octgear/profile.json. Do not edit by hand.
#pragma once

#include <Arduino.h>

namespace Config {

constexpr uint8_t PHYSICAL_KEY_COUNT = ${physicalKeyCount};
constexpr uint8_t ENCODER_CONTROL_COUNT = ${encoderControlCount};
constexpr uint8_t KEY_COUNT = PHYSICAL_KEY_COUNT + ENCODER_CONTROL_COUNT;
constexpr uint8_t LAYER_COUNT = ${profile.layerCount};
constexpr uint8_t DEFAULT_ENABLED_LAYER_MASK = ${hexByte(defaultEnabledLayerMask)};
constexpr uint8_t DEFAULT_LAYER_COLORS[LAYER_COUNT][3] = {
${profile.defaultLayerColors.map((color) => `  { ${color.join(", ")} },`).join("\n")}
};
constexpr uint8_t MATRIX_ROW_COUNT = ${profile.matrix.rows.length};
constexpr uint8_t MATRIX_COLUMN_COUNT = ${profile.matrix.columns.length};

using KeyMask = uint16_t;

constexpr uint8_t MATRIX_ROW_PINS[MATRIX_ROW_COUNT] = {
  ${matrixRowPins.join(", ")}
};

constexpr uint8_t MATRIX_COLUMN_PINS[MATRIX_COLUMN_COUNT] = {
  ${matrixColumnPins.join(", ")}
};

constexpr uint8_t KEY_MATRIX_ROWS[PHYSICAL_KEY_COUNT] = {
  ${keyMatrixRows.join(", ")}
};

constexpr uint8_t KEY_MATRIX_COLUMNS[PHYSICAL_KEY_COUNT] = {
  ${keyMatrixColumns.join(", ")}
};

constexpr uint8_t ENCODER_A_PIN = ${profile.encoder.aPin};
constexpr uint8_t ENCODER_COMMON_PIN = ${profile.encoder.commonPin};
constexpr uint8_t ENCODER_B_PIN = ${profile.encoder.bPin};
constexpr uint8_t ENCODER_SWITCH_PIN = ${profile.encoder.switchPin};
constexpr uint8_t ENCODER_CCW_KEY_INDEX = ${encoderControlById["enc-ccw"].firmwareIndex};
constexpr uint8_t ENCODER_CW_KEY_INDEX = ${encoderControlById["enc-cw"].firmwareIndex};
constexpr uint8_t ENCODER_SWITCH_KEY_INDEX = ${encoderControlById["enc-sw"].firmwareIndex};
constexpr int8_t ENCODER_STEPS_PER_DETENT = ${profile.encoder.stepsPerDetent};
constexpr bool ENCODER_REVERSED = ${profile.encoder.reversed ? "true" : "false"};

}  // namespace Config
`;
}

function webConfig() {
  const keyControls = profile.keys.map((key) =>
    `  { id: ${json(key.id)}, label: ${json(key.label)}, bit: ${key.firmwareIndex}, kind: "key" },`
  ).join("\n");
  const encoderControls = profile.encoder.controls.map((control) =>
    `  { id: ${json(control.id)}, label: ${json(control.label)}, bit: ${control.firmwareIndex}, kind: ${json(control.kind)} },`
  ).join("\n");

  return `// Generated from hardware/octgear/profile.json. Do not edit by hand.

const keyControls = [
${keyControls}
] as const;

const encoderControls = [
${encoderControls}
] as const;

export const HARDWARE_CONFIG = {
  productName: ${json(profile.productName)},
  physicalKeyCount: keyControls.length,
  keyCount: keyControls.length + encoderControls.length,
  layerCount: ${profile.layerCount},
  defaultEnabledLayers: ${json(profile.defaultEnabledLayers)} as readonly number[],
  defaultLayerColors: ${json(profile.defaultLayerColors)} as readonly (readonly [number, number, number])[],
  matrix: {
    rowCount: ${profile.matrix.rows.length},
    columnCount: ${profile.matrix.columns.length},
    diodeDirection: ${json(profile.matrix.diodeDirection)},
  },
  externalRgbLed: ${profile.externalRgbLed ? "true" : "false"},
  oled: ${profile.oled ? "true" : "false"},
  encoder: {
    enabled: ${profile.encoder.enabled ? "true" : "false"},
    pinCount: ${profile.encoder.pinCount},
    reversed: ${profile.encoder.reversed ? "true" : "false"},
    controls: encoderControls,
  },
  keyPins: keyControls,
  controls: [...keyControls, ...encoderControls],
} as const;
`;
}

function pinoutMarkdown() {
  const keyRows = profile.keys.map((key) =>
    `| ${key.logicalName} | ${key.firmwareIndex} | Row ${key.row + 1} / Column ${key.column + 1} | ${key.notes} |`
  ).join("\n");
  const matrixRows = profile.matrix.rows.map((row) =>
    `| ${row.label} | ${row.firmwareIndex} | ${row.gpio} | Scan output; selected row is OUTPUT LOW |`
  ).join("\n");
  const matrixColumnRows = profile.matrix.columns.map((column) =>
    `| ${column.label} | ${column.firmwareIndex} | ${column.gpio} | INPUT_PULLUP |`
  ).join("\n");

  return `# Pinout

現行8キー + ロータリーエンコーダ版のファームウェアとPCB設計を合わせるためのピン割り当てです。

この表は \`hardware/octgear/profile.json\` から生成します。

## Key Matrix

8 keys form a 2 x 4 matrix without diodes. Columns use \`INPUT_PULLUP\`; firmware drives only the selected row \`OUTPUT LOW\` and leaves the other row high impedance.

| Logical key | Firmware index | Matrix position | Notes |
| --- | ---: | --- | --- |
${keyRows}

| Row | Index | GPIO | Mode |
| --- | ---: | ---: | --- |
${matrixRows}

| Column | Index | GPIO | Mode |
| --- | ---: | ---: | --- |
${matrixColumnRows}

Because the matrix has no diodes, rectangular multi-key combinations are electrically ambiguous. Firmware holds the previous stable matrix state while an ambiguous combination is present, preventing phantom key output.

## Rotary Encoder

Encoder A/B and SW are independent from the key matrix. A/B/SW use \`INPUT_PULLUP\`; Common is a dedicated \`OUTPUT LOW\` on GPIO ${profile.encoder.commonPin}.

The compiled default direction is ${profile.encoder.reversed ? "reversed" : "standard"}. Remapper can change and persist the direction without rebuilding firmware.

| Signal | GPIO | Firmware control index |
| --- | ---: | ---: |
| A | ${profile.encoder.aPin} | CCW/CW: ${encoderControlById["enc-ccw"].firmwareIndex}/${encoderControlById["enc-cw"].firmwareIndex} |
| Common | ${profile.encoder.commonPin} | - |
| B | ${profile.encoder.bPin} | CCW/CW: ${encoderControlById["enc-ccw"].firmwareIndex}/${encoderControlById["enc-cw"].firmwareIndex} |
| SW | ${profile.encoder.switchPin} | ${encoderControlById["enc-sw"].firmwareIndex} |

## Removed Parts

新ハードウェアでは以下を使いません。

- External RGB LED / WS2812B
- OLED

状態表示は本体LEDのみで行います。
`;
}

function requireArray(value, name) {
  if (!Array.isArray(value)) {
    throw new Error(`${name} must be an array`);
  }
}

function requireObject(value, name) {
  if (value === null || typeof value !== "object" || Array.isArray(value)) {
    throw new Error(`${name} must be an object`);
  }
}

function requireInteger(value, name) {
  if (!Number.isInteger(value)) {
    throw new Error(`${name} must be an integer`);
  }
}

function json(value) {
  return JSON.stringify(value);
}

function hexByte(value) {
  return `0x${value.toString(16).toUpperCase().padStart(2, "0")}`;
}
