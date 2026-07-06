export type HidInputReportEvent = Event & {
  readonly device: HidDevice;
  readonly reportId: number;
  readonly data: DataView;
};

export type HidConnectionEvent = Event & {
  readonly device: HidDevice;
};

export type HidDevice = EventTarget & {
  readonly opened: boolean;
  readonly productName: string;
  readonly vendorId: number;
  readonly productId: number;
  open(): Promise<void>;
  close(): Promise<void>;
  sendReport(reportId: number, data: BufferSource): Promise<void>;
  sendFeatureReport(reportId: number, data: BufferSource): Promise<void>;
  receiveFeatureReport(reportId: number): Promise<DataView>;
  addEventListener(
    type: "inputreport",
    listener: (event: HidInputReportEvent) => void,
  ): void;
  removeEventListener(
    type: "inputreport",
    listener: (event: HidInputReportEvent) => void,
  ): void;
};

export type HidApi = {
  requestDevice(options: {
    filters: Array<{
      vendorId?: number;
      productId?: number;
      usagePage?: number;
      usage?: number;
    }>;
  }): Promise<HidDevice[]>;
  getDevices(): Promise<HidDevice[]>;
  addEventListener(
    type: "disconnect",
    listener: (event: HidConnectionEvent) => void,
  ): void;
  removeEventListener(
    type: "disconnect",
    listener: (event: HidConnectionEvent) => void,
  ): void;
};

export type HidNavigator = Navigator & {
  hid?: HidApi;
};
