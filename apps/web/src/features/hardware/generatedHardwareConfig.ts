// Generated from hardware/octgear/profile.json. Do not edit by hand.

const keyControls = [
  { id: "k1", label: "K1", bit: 0, kind: "key" },
  { id: "k2", label: "K2", bit: 1, kind: "key" },
  { id: "k3", label: "K3", bit: 2, kind: "key" },
  { id: "k4", label: "K4", bit: 3, kind: "key" },
  { id: "k5", label: "K5", bit: 4, kind: "key" },
  { id: "k6", label: "K6", bit: 5, kind: "key" },
  { id: "k7", label: "K7", bit: 6, kind: "key" },
  { id: "k8", label: "K8", bit: 7, kind: "key" },
] as const;

const encoderControls = [
  { id: "enc-ccw", label: "CCW", bit: 8, kind: "encoder" },
  { id: "enc-cw", label: "CW", bit: 9, kind: "encoder" },
  { id: "enc-sw", label: "SW", bit: 10, kind: "encoderSwitch" },
] as const;

export const HARDWARE_CONFIG = {
  productName: "OctGear",
  physicalKeyCount: keyControls.length,
  keyCount: keyControls.length + encoderControls.length,
  layerCount: 8,
  defaultEnabledLayers: [0,1] as readonly number[],
  defaultLayerColors: [[255,255,255],[255,0,0],[0,180,255],[0,255,80],[255,160,0],[180,0,255],[0,255,220],[255,70,140]] as readonly (readonly [number, number, number])[],
  matrix: {
    rowCount: 2,
    columnCount: 4,
    diodeDirection: "none",
  },
  externalRgbLed: true,
  oled: false,
  encoder: {
    enabled: true,
    pinCount: 4,
    reversed: false,
    controls: encoderControls,
  },
  keyPins: keyControls,
  controls: [...keyControls, ...encoderControls],
} as const;
