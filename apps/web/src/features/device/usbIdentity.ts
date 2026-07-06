export const OCTGEAR_USB = {
  vendorId: 0xcafe,
  productId: 0xc608,
  manufacturerName: "OctGear",
  productName: "OctGear",
} as const;

export function formatUsbId(value: number) {
  return `0x${value.toString(16).padStart(4, "0").toUpperCase()}`;
}
