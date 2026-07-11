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
const keyPins = profile.keys.map((key) => key.gpio);
const virtualGroundPins = profile.virtualGrounds.map((rail) => rail.gpio);
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
  requireArray(value.keys, "keys");
  requireArray(value.virtualGrounds, "virtualGrounds");
  requireObject(value.encoder, "encoder");

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

  value.keys.forEach((key, index) => {
    requireInteger(key.firmwareIndex, `keys[${index}].firmwareIndex`);
    requireInteger(key.gpio, `keys[${index}].gpio`);
    if (key.firmwareIndex !== index) {
      throw new Error(`keys[${index}].firmwareIndex must be ${index}`);
    }
  });

  value.virtualGrounds.forEach((rail, index) => {
    requireInteger(rail.firmwareIndex, `virtualGrounds[${index}].firmwareIndex`);
    requireInteger(rail.gpio, `virtualGrounds[${index}].gpio`);
    if (rail.firmwareIndex !== index) {
      throw new Error(`virtualGrounds[${index}].firmwareIndex must be ${index}`);
    }
  });

  if (value.encoder.enabled) {
    requireInteger(value.encoder.aPin, "encoder.aPin");
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
constexpr uint8_t VIRTUAL_GROUND_COUNT = ${profile.virtualGrounds.length};

using KeyMask = uint16_t;

constexpr uint8_t KEY_PINS[PHYSICAL_KEY_COUNT] = {
  ${keyPins.join(", ")}
};

constexpr uint8_t VIRTUAL_GROUND_PINS[VIRTUAL_GROUND_COUNT] = {
  ${virtualGroundPins.join(", ")}
};

constexpr uint8_t ENCODER_A_PIN = ${profile.encoder.aPin};
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
  virtualGroundCount: ${profile.virtualGrounds.length},
  externalRgbLed: ${profile.externalRgbLed ? "true" : "false"},
  oled: ${profile.oled ? "true" : "false"},
  encoder: {
    enabled: ${profile.encoder.enabled ? "true" : "false"},
    pinCount: ${profile.encoder.pinCount},
    controls: encoderControls,
  },
  keyPins: keyControls,
  controls: [...keyControls, ...encoderControls],
} as const;
`;
}

function pinoutMarkdown() {
  const keyRows = profile.keys.map((key) =>
    `| ${key.logicalName} | ${key.firmwareIndex} | ${key.gpio} | ${key.virtualGround} | ${key.notes} |`
  ).join("\n");
  const encoderRows = profile.encoder.controls.map((control) => {
    const gpio = control.id === "enc-sw"
      ? `${profile.encoder.switchPin}`
      : `A: ${profile.encoder.aPin} / B: ${profile.encoder.bPin}`;
    return `| ${control.label === "SW" ? "Encoder SW" : `Encoder ${control.label}`} | ${control.firmwareIndex} | ${gpio} | ${control.notes} |`;
  }).join("\n");
  const virtualGroundRows = profile.virtualGrounds.map((rail) =>
    `| ${rail.rail} | ${rail.firmwareIndex} | ${rail.gpio} | ${rail.notes} |`
  ).join("\n");

  return `# Pinout

現行8キー + ロータリーエンコーダ版のファームウェアとPCB設計を合わせるためのピン割り当てです。

この表は \`hardware/octgear/profile.json\` から生成します。

## Key Inputs

Direct入力は8本です。各入力はファームウェアで \`INPUT_PULLUP\` に設定します。

| Logical key | Firmware index | GPIO | Virtual ground rail | Notes |
| --- | ---: | --- | --- | --- |
${keyRows}

## Rotary Encoder

エンコーダのA/B相は回転方向ごとの仮想キーとして扱います。SWは通常のDirect入力と同じく \`INPUT_PULLUP\` です。

| Control | Firmware index | GPIO | Notes |
| --- | ---: | --- | --- |
${encoderRows}

## Virtual Ground

仮想GND用GPIOは2本です。どちらもファームウェアで常時 \`OUTPUT LOW\` に設定します。

| Rail | Firmware index | GPIO | Notes |
| --- | ---: | --- | --- |
${virtualGroundRows}

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
