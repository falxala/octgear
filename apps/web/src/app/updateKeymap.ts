import {
  createBlankAssignment,
  normalizeAssignment,
  sameAssignment,
  type KeyAssignment,
} from "../features/keymap/keymapTypes";

export function updateKeymap(
  keymap: KeyAssignment[][],
  layer: number,
  keyIndex: number,
  assignment: KeyAssignment,
) {
  return keymap.map((layerAssignments, layerIndex) =>
    layerIndex === layer
      ? layerAssignments.map((current, currentKeyIndex) =>
          currentKeyIndex === keyIndex ? assignment : current,
        )
      : layerAssignments,
  );
}

export function updateKeymapKeyAcrossLayers(
  keymap: KeyAssignment[][],
  keyIndex: number,
  assignment: KeyAssignment,
) {
  return keymap.map((layerAssignments) =>
    layerAssignments.map((current, currentKeyIndex) =>
      currentKeyIndex === keyIndex ? assignment : current,
    ),
  );
}

export function applyLayerCycleOverrides(keymap: KeyAssignment[][]) {
  const layerCycleAssignments = new Map<number, KeyAssignment>();

  keymap.forEach((layerAssignments) => {
    layerAssignments.forEach((assignment, keyIndex) => {
      if (assignment.kind === "layerCycle" && !layerCycleAssignments.has(keyIndex)) {
        layerCycleAssignments.set(keyIndex, assignment);
      }
    });
  });

  if (layerCycleAssignments.size === 0) {
    return keymap;
  }

  return keymap.map((layerAssignments) =>
    layerAssignments.map((assignment, keyIndex) =>
      layerCycleAssignments.get(keyIndex) ?? assignment,
    ),
  );
}

export function createBlankKeymap(layerCount: number, keyCount: number) {
  return Array.from({ length: layerCount }, () =>
    Array.from({ length: keyCount }, createBlankAssignment),
  );
}

export function expandKeymap(
  keymap: KeyAssignment[][],
  layerCount: number,
  keyCount: number,
) {
  const next = createBlankKeymap(layerCount, keyCount);

  keymap.forEach((layerAssignments, layer) => {
    if (layer >= layerCount) {
      return;
    }

    layerAssignments.forEach((assignment, keyIndex) => {
      if (keyIndex < keyCount) {
        next[layer][keyIndex] = assignment;
      }
    });
  });

  return next;
}

export function normalizeKeymapForDevice(
  keymap: KeyAssignment[][],
  layerCount: number,
  keyCount: number,
) {
  return Array.from({ length: layerCount }, (_, layer) =>
    Array.from({ length: keyCount }, (_, keyIndex) =>
      normalizeAssignment(keymap[layer]?.[keyIndex] ?? createBlankAssignment()),
    ),
  );
}

export function collectChangedAssignments(
  readKeymap: KeyAssignment[][],
  writeKeymap: KeyAssignment[][],
) {
  const targets: Array<{
    layer: number;
    keyIndex: number;
    assignment: KeyAssignment;
  }> = [];

  writeKeymap.forEach((layerAssignments, layer) => {
    layerAssignments.forEach((assignment, keyIndex) => {
      const current = readKeymap[layer]?.[keyIndex] ?? createBlankAssignment();
      if (!sameAssignment(current, assignment)) {
        targets.push({ layer, keyIndex, assignment });
      }
    });
  });

  return targets;
}
