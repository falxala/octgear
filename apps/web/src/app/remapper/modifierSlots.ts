export function createModifierSlotsFromMask(mask: number) {
  const slots: number[] = [];
  const modifierBits = [0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80];

  for (const bit of modifierBits) {
    if ((mask & bit) !== 0) {
      slots.push(bit);
    }
  }

  while (slots.length < 3) {
    slots.push(0);
  }

  return slots.slice(0, 3);
}

export function createModifierMaskFromSlots(slots: number[]) {
  let mask = 0;

  for (const slot of slots) {
    if (slot !== 0) {
      mask |= slot;
    }
  }

  return mask;
}
