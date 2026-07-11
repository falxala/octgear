import { HARDWARE_CONFIG } from "../hardware/hardwareConfig";
import {
  createBlankAssignment,
  createConsumerAssignment,
  createKeyboardAssignment,
  createLayerCycleAssignment,
  createMomentaryLayerAssignment,
  type DeviceKeymap,
} from "./keymapTypes";

export function createInitialKeymap(): DeviceKeymap {
  const keymap = Array.from({ length: HARDWARE_CONFIG.layerCount }, () =>
    Array.from({ length: HARDWARE_CONFIG.keyCount }, createBlankAssignment),
  );

  applyLayerAssignments(keymap, 0, [
    createLayerCycleAssignment(),
    createConsumerAssignment(0x00e9),
    createConsumerAssignment(0x00ea),
    createConsumerAssignment(0x00e2),
    createMomentaryLayerAssignment(1),
    createConsumerAssignment(0x00b6),
    createConsumerAssignment(0x00cd),
    createConsumerAssignment(0x00b5),
    createConsumerAssignment(0x00ea),
    createConsumerAssignment(0x00e9),
    createConsumerAssignment(0x00cd),
  ]);

  applyLayerAssignments(keymap, 1, [
    createKeyboardAssignment(0x14),
    createKeyboardAssignment(0x1a),
    createKeyboardAssignment(0x08),
    createKeyboardAssignment(0x15),
    createKeyboardAssignment(0x04),
    createKeyboardAssignment(0x16),
    createKeyboardAssignment(0x07),
    createKeyboardAssignment(0x09),
  ]);

  applyLayerAssignments(keymap, 2, [
    createKeyboardAssignment(0x52),
    createKeyboardAssignment(0x51),
    createKeyboardAssignment(0x50),
    createKeyboardAssignment(0x4f),
    createKeyboardAssignment(0x4a),
    createKeyboardAssignment(0x4d),
    createKeyboardAssignment(0x4b),
    createKeyboardAssignment(0x4e),
  ]);

  for (const layerAssignments of keymap) {
    layerAssignments[0] = createLayerCycleAssignment();
  }

  return keymap;
}

function applyLayerAssignments(
  keymap: DeviceKeymap,
  layer: number,
  assignments: DeviceKeymap[number],
) {
  assignments.forEach((assignment, keyIndex) => {
    if (keyIndex < keymap[layer].length) {
      keymap[layer][keyIndex] = assignment;
    }
  });
}
