import type { DeviceState } from "../../features/device/deviceCommands";
import { OCTGEAR_USB, formatUsbId } from "../../features/device/usbIdentity";
import { HARDWARE_CONFIG } from "../../features/hardware/hardwareConfig";
import { t } from "../../shared/i18n";

type HardwarePanelProps = {
  deviceState: DeviceState | null;
};

export function HardwarePanel({ deviceState }: HardwarePanelProps) {
  return (
    <aside className="panel hardware-panel">
      <div className="panel-meta">
        <span className="panel-kicker">{t.hardware.kicker}</span>
        <h2>{t.hardware.title}</h2>
      </div>
      <dl>
        <div>
          <dt>{t.hardware.keys}</dt>
          <dd>{HARDWARE_CONFIG.physicalKeyCount}</dd>
        </div>
        <div>
          <dt>{t.hardware.encoder}</dt>
          <dd>{t.hardware.encoderValue(HARDWARE_CONFIG.encoder.pinCount)}</dd>
        </div>
        <div>
          <dt>{t.hardware.virtualGround}</dt>
          <dd>{HARDWARE_CONFIG.virtualGroundCount}</dd>
        </div>
        <div>
          <dt>{t.hardware.deviceLayer}</dt>
          <dd>{deviceState?.activeLayer ?? "-"}</dd>
        </div>
        <div>
          <dt>{t.hardware.reportKeys}</dt>
          <dd>{deviceState?.keyCount ?? "-"}</dd>
        </div>
        <div>
          <dt>{t.hardware.usbId}</dt>
          <dd>
            {formatUsbId(OCTGEAR_USB.vendorId)}:{formatUsbId(OCTGEAR_USB.productId)}
          </dd>
        </div>
        <div>
          <dt>{t.hardware.externalRgb}</dt>
          <dd>{t.hardware.none}</dd>
        </div>
        <div>
          <dt>{t.hardware.oled}</dt>
          <dd>{t.hardware.none}</dd>
        </div>
      </dl>
    </aside>
  );
}
