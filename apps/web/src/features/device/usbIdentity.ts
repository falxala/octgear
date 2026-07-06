export const CYBORG_MINI_USB = {
  vendorId: 0xcafe,
  productId: 0xc608,
  manufacturerName: "Cyborg Project",
  productName: "Cyborg Mini 8 Keys",
} as const;

export function formatUsbId(value: number) {
  return `0x${value.toString(16).padStart(4, "0").toUpperCase()}`;
}
