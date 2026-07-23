import type { DeviceState } from "../../features/device/deviceCommands";
import { OCTGEAR_USB, formatUsbId } from "../../features/device/usbIdentity";
import { HARDWARE_CONFIG } from "../../features/hardware/hardwareConfig";
import { t } from "../../shared/i18n";

type HardwarePanelProps = {
  deviceState: DeviceState | null;
  encoderDirectionUpdating: boolean;
  statusLedBrightness: number;
  statusLedBrightnessUpdating: boolean;
  onEncoderReversedChange: (reversed: boolean) => void;
  onStatusLedBrightnessChange: (brightness: number) => void;
  onStatusLedBrightnessApply: () => void;
};

export function HardwarePanel({
  deviceState,
  encoderDirectionUpdating,
  statusLedBrightness,
  statusLedBrightnessUpdating,
  onEncoderReversedChange,
  onStatusLedBrightnessChange,
  onStatusLedBrightnessApply,
}: HardwarePanelProps) {
  const brightnessSupported = deviceState?.statusLedBrightnessSupported ?? false;
  const brightnessChanged =
    brightnessSupported && statusLedBrightness !== deviceState?.statusLedBrightness;

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
          <dt>{t.hardware.encoderDirection}</dt>
          <dd>
            <label className="hardware-toggle">
              <input
                type="checkbox"
                checked={deviceState?.encoderReversed ?? HARDWARE_CONFIG.encoder.reversed}
                disabled={!deviceState || encoderDirectionUpdating}
                onChange={(event) => onEncoderReversedChange(event.target.checked)}
              />
              <span>{t.hardware.encoderReversed}</span>
            </label>
          </dd>
        </div>
        <div className="hardware-brightness-row">
          <dt>{t.hardware.statusLedBrightness}</dt>
          <dd>
            <div className="hardware-brightness-control">
              <input
                type="range"
                min={0}
                max={HARDWARE_CONFIG.statusLedBrightness.max}
                value={statusLedBrightness}
                aria-label={t.hardware.statusLedBrightness}
                disabled={!brightnessSupported || statusLedBrightnessUpdating}
                onChange={(event) => onStatusLedBrightnessChange(Number(event.target.value))}
              />
              <input
                type="number"
                min={0}
                max={HARDWARE_CONFIG.statusLedBrightness.max}
                value={statusLedBrightness}
                aria-label={t.hardware.statusLedBrightnessValue}
                disabled={!brightnessSupported || statusLedBrightnessUpdating}
                onChange={(event) => onStatusLedBrightnessChange(Number(event.target.value))}
              />
              <button
                type="button"
                disabled={!brightnessChanged || statusLedBrightnessUpdating}
                onClick={onStatusLedBrightnessApply}
              >
                {statusLedBrightnessUpdating ? t.hardware.applying : t.hardware.apply}
              </button>
            </div>
            <small>
              {brightnessSupported
                ? t.hardware.statusLedBrightnessRange(HARDWARE_CONFIG.statusLedBrightness.max)
                : t.hardware.statusLedBrightnessUnsupported}
            </small>
          </dd>
        </div>
        <div>
          <dt>{t.hardware.matrix}</dt>
          <dd>{t.hardware.matrixValue(
            deviceState?.matrixRowCount ?? HARDWARE_CONFIG.matrix.rowCount,
            HARDWARE_CONFIG.matrix.columnCount,
            HARDWARE_CONFIG.matrix.diodeDirection,
          )}</dd>
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
          <dd>
            {HARDWARE_CONFIG.externalRgbLed
              ? t.hardware.externalRgbValue(HARDWARE_CONFIG.externalRgbLedCount)
              : t.hardware.none}
          </dd>
        </div>
        <div>
          <dt>{t.hardware.oled}</dt>
          <dd>{t.hardware.none}</dd>
        </div>
      </dl>
    </aside>
  );
}
