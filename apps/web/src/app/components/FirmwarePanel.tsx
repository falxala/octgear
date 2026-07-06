import { t } from "../../shared/i18n";

type FirmwarePanelProps = {
  connected: boolean;
  firmwareInstallSupported: boolean;
  firmwareStatus: string;
  onEnterBootloader: () => void;
  onInstallFirmware: () => void;
  onDownloadFirmware: () => void;
};

export function FirmwarePanel({
  connected,
  firmwareInstallSupported,
  firmwareStatus,
  onEnterBootloader,
  onInstallFirmware,
  onDownloadFirmware,
}: FirmwarePanelProps) {
  return (
    <div className="firmware-panel">
      <div className="firmware-summary">
        <h2>{t.firmware.title}</h2>
        <span>{firmwareStatus}</span>
      </div>
      <div className="firmware-help">
        <section>
          <h3>{t.firmware.normalUpdate}</h3>
          <ol>
            {t.firmware.normalSteps.map((step) => (
              <li key={step}>{step}</li>
            ))}
          </ol>
        </section>
        <section>
          <h3>{t.firmware.recovery}</h3>
          <ol>
            {t.firmware.recoverySteps.map((step) => (
              <li key={step}>{step}</li>
            ))}
          </ol>
        </section>
      </div>
      <div className="firmware-actions">
        <button type="button" className="primary-action" onClick={onEnterBootloader} disabled={!connected}>
          {t.firmware.bootsel}
        </button>
        <button type="button" onClick={onInstallFirmware} disabled={!firmwareInstallSupported}>
          {t.firmware.installUf2}
        </button>
        <button type="button" onClick={onDownloadFirmware}>
          {t.firmware.downloadUf2}
        </button>
      </div>
    </div>
  );
}
