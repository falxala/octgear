import type { HidConnectionEvent, HidDevice, HidInputReportEvent, HidNavigator } from "./webHidTypes";
import { CONFIG_REPORT_ID } from "./hidProtocol";
import { CYBORG_MINI_USB } from "./usbIdentity";
import { t } from "../../shared/i18n";

export type ConfigReportListener = (report: Uint8Array) => void;

export class WebHidTransport {
  private device: HidDevice | null = null;

  get connected() {
    return this.device?.opened ?? false;
  }

  async requestDevice() {
    const hid = this.getHidApi();
    console.info("[hid] requesting device with filters", [CYBORG_MINI_USB]);
    const devices = await hid.requestDevice({
      filters: [
        {
          vendorId: CYBORG_MINI_USB.vendorId,
          productId: CYBORG_MINI_USB.productId,
        },
      ],
    });

    if (devices.length > 0) {
      this.device = devices[0];
      this.logSelectedDevice(this.device);
      return this.device;
    }

    throw new Error(t.device.notFound);
  }

  async open() {
    if (!this.device) {
      throw new Error(t.device.missingDevice);
    }

    if (!this.device.opened) {
      await this.device.open();
    }
  }

  async close() {
    const device = this.device;
    this.device = null;

    if (device?.opened) {
      await device.close();
    }
  }

  async requestConfigReport(report: Uint8Array, timeoutMs = 1000) {
    const device = this.requireOpenDevice();

    return new Promise<Uint8Array>((resolve, reject) => {
      const timeout = window.setTimeout(() => {
        cleanup();
        reject(new Error(t.device.timeout));
      }, timeoutMs);

      const expectedCommand = report[0];
      const listener = (event: HidInputReportEvent) => {
        if (event.reportId !== CONFIG_REPORT_ID) {
          return;
        }

        const view = new Uint8Array(event.data.buffer, event.data.byteOffset, event.data.byteLength);
        const raw = Uint8Array.from(view);
        if (raw[0] !== expectedCommand) {
          return;
        }

        cleanup();
        resolve(raw);
      };

      const cleanup = () => {
        window.clearTimeout(timeout);
        device.removeEventListener("inputreport", listener);
      };

      device.addEventListener("inputreport", listener);
      device.sendReport(CONFIG_REPORT_ID, toArrayBuffer(report)).catch((error) => {
        cleanup();
        reject(error);
      });
    });
  }

  async sendConfigReport(report: Uint8Array) {
    const device = this.requireOpenDevice();
    await device.sendReport(CONFIG_REPORT_ID, toArrayBuffer(report));
  }

  addConfigReportListener(listener: ConfigReportListener) {
    const device = this.requireOpenDevice();
    const inputListener = (event: HidInputReportEvent) => {
      if (event.reportId !== CONFIG_REPORT_ID) {
        return;
      }

      const view = new Uint8Array(event.data.buffer, event.data.byteOffset, event.data.byteLength);
      listener(Uint8Array.from(view));
    };

    device.addEventListener("inputreport", inputListener);

    return () => {
      device.removeEventListener("inputreport", inputListener);
    };
  }

  addDisconnectListener(listener: () => void) {
    const hid = this.getHidApi();
    const disconnectListener = (event: HidConnectionEvent) => {
      if (event.device !== this.device) {
        return;
      }

      this.device = null;
      listener();
    };

    hid.addEventListener("disconnect", disconnectListener);

    return () => {
      hid.removeEventListener("disconnect", disconnectListener);
    };
  }

  private getHidApi() {
    const hid = (navigator as HidNavigator).hid;

    if (!hid) {
      throw new Error(t.device.unsupportedWebHid);
    }

    return hid;
  }

  private requireOpenDevice() {
    if (!this.device || !this.device.opened) {
      throw new Error(t.device.disconnected);
    }

    return this.device;
  }

  private logSelectedDevice(device: HidDevice) {
    console.info("[hid] selected device", {
      productName: device.productName,
      vendorId: `0x${device.vendorId.toString(16).padStart(4, "0").toUpperCase()}`,
      productId: `0x${device.productId.toString(16).padStart(4, "0").toUpperCase()}`,
    });
  }
}

function toArrayBuffer(bytes: Uint8Array) {
  const buffer = new ArrayBuffer(bytes.byteLength);
  new Uint8Array(buffer).set(bytes);
  return buffer;
}
