import { useEffect, useMemo, useState } from "react";

import { homeUrl, remapperUrl } from "../appUrls";
import { useDeviceSession } from "../hooks/useDeviceSession";
import {
  getDeviceState,
  runDiagnosticReportTest,
  runDiagnosticStorageTest,
  sendRemapperHeartbeat,
  subscribeDeviceKeyEvents,
  type DeviceState,
} from "../../features/device/deviceCommands";
import { WebHidTransport } from "../../features/device/webHidTransport";
import { HARDWARE_CONFIG } from "../../features/hardware/hardwareConfig";
import { t } from "../../shared/i18n";

export function DiagnosticsApp() {
  const transport = useMemo(() => new WebHidTransport(), []);
  const [status, setStatus] = useState<string>(t.connection.initialStatus);
  const [deviceState, setDeviceState] = useState<DeviceState | null>(null);
  const [reportTestStatus, setReportTestStatus] = useState<"idle" | "running" | "passed" | "failed">("idle");
  const [reportTestDetail, setReportTestDetail] = useState("-");
  const [storageTestStatus, setStorageTestStatus] = useState<"idle" | "running" | "passed" | "failed">("idle");
  const [storageTestDetail, setStorageTestDetail] = useState("-");
  const [testedKeys, setTestedKeys] = useState<boolean[]>(() =>
    Array.from({ length: HARDWARE_CONFIG.keyCount }, () => false),
  );
  const [lastKey, setLastKey] = useState<number | null>(null);
  const connected = deviceState !== null && transport.connected;
  const testedCount = testedKeys.filter(Boolean).length;
  const allKeysPassed = testedCount === testedKeys.length;
  const disconnectDevice = useDeviceSession(transport, connected, () => {
    setDeviceState(null);
    setLastKey(null);
    setReportTestStatus("idle");
    setReportTestDetail("-");
    setStorageTestStatus("idle");
    setStorageTestDetail("-");
    setStatus(t.connection.initialStatus);
  });

  useEffect(() => {
    if (!connected || !deviceState) {
      return;
    }

    return subscribeDeviceKeyEvents(transport, (event) => {
      if (!event.pressed || event.keyIndex >= deviceState.keyCount) {
        return;
      }

      setLastKey(event.keyIndex);
      setTestedKeys((current) =>
        current.map((tested, index) => (index === event.keyIndex ? true : tested)),
      );
    });
  }, [connected, deviceState, transport]);

  async function connectDevice() {
    try {
      const device = await transport.requestDevice();
      await transport.open();
      await sendRemapperHeartbeat(transport);
      const state = await getDeviceState(transport);
      setDeviceState(state);
      setTestedKeys(Array.from({ length: state.keyCount }, () => false));
      setLastKey(null);
      await runReportTest();
      await runStorageTest();
      setStatus(t.connection.connectedTo(device.productName || t.device.fallbackName));
    } catch (error) {
      setStatus(error instanceof Error ? error.message : t.connection.connectFailed);
    }
  }

  async function runReportTest() {
    if (!transport.connected) {
      setReportTestStatus("failed");
      setReportTestDetail(t.connection.deviceNotConnected);
      return;
    }

    try {
      setReportTestStatus("running");
      setReportTestDetail(t.diagnostics.reportTesting);
      const result = await runDiagnosticReportTest(transport);
      setReportTestStatus("passed");
      setReportTestDetail(t.diagnostics.reportTestPassed(result.signature, result.version));
    } catch (error) {
      setReportTestStatus("failed");
      setReportTestDetail(error instanceof Error ? error.message : t.diagnostics.reportTestFailed);
    }
  }

  async function runStorageTest() {
    if (!transport.connected) {
      setStorageTestStatus("failed");
      setStorageTestDetail(t.connection.deviceNotConnected);
      return;
    }

    try {
      setStorageTestStatus("running");
      setStorageTestDetail(t.diagnostics.storageTesting);
      const result = await runDiagnosticStorageTest(transport);
      setStorageTestStatus("passed");
      setStorageTestDetail(t.diagnostics.storageTestPassed(result.layerCount, result.keyCount));
    } catch (error) {
      setStorageTestStatus("failed");
      setStorageTestDetail(error instanceof Error ? error.message : t.diagnostics.storageTestFailed);
    }
  }

  function resetDiagnostics() {
    setTestedKeys(Array.from({ length: deviceState?.keyCount ?? HARDWARE_CONFIG.keyCount }, () => false));
    setLastKey(null);
  }

  return (
    <main className="app-shell">
      <header className="topbar">
        <div className="brand">
          <img src={`${import.meta.env.BASE_URL}cy.png`} alt="" />
          <div className="brand-copy">
            <span className="eyebrow">{t.diagnostics.kicker}</span>
            <h1>{t.diagnostics.title}</h1>
            <p>{t.diagnostics.description}</p>
          </div>
        </div>
        <div className="connection">
          <div className="connection-meta">
            <span className={connected ? "status-badge online" : "status-badge offline"}>
              {connected ? t.connection.connected : t.connection.idle}
            </span>
            <span className="connection-text">{status}</span>
          </div>
          <div className="connection-actions">
            <a className="ghost-button nav-button" href={homeUrl}>
              {t.home.backHome}
            </a>
            <a className="ghost-button nav-button" href={remapperUrl}>
              {t.home.openRemapper}
            </a>
            <button type="button" onClick={connected ? disconnectDevice : connectDevice}>
              {connected ? t.connection.disconnect : t.connection.connect}
            </button>
          </div>
        </div>
      </header>

      <section className="diagnostics-workspace" aria-label={t.diagnostics.title}>
        <section className="panel diagnostics-panel">
          <div className="panel-heading">
            <div className="panel-meta">
              <span className="panel-kicker">{t.diagnostics.keyCheckKicker}</span>
              <h2>{t.diagnostics.keyCheckTitle}</h2>
            </div>
            <button type="button" className="ghost-button" onClick={resetDiagnostics}>
              {t.diagnostics.reset}
            </button>
            <button type="button" className="ghost-button" onClick={() => void runReportTest()} disabled={!connected}>
              {t.diagnostics.runReportTest}
            </button>
            <button type="button" className="ghost-button" onClick={() => void runStorageTest()} disabled={!connected}>
              {t.diagnostics.runStorageTest}
            </button>
          </div>

          <div className="diagnostics-summary">
            <strong>{allKeysPassed ? t.diagnostics.pass : t.diagnostics.waiting}</strong>
            <span>{t.diagnostics.progress(testedCount, testedKeys.length)}</span>
            <span>{lastKey === null ? t.diagnostics.noLastKey : t.diagnostics.lastKey(lastKey + 1)}</span>
          </div>

          <p className="diagnostics-warning">{t.diagnostics.storageWriteWarning}</p>

          <div className="diagnostic-key-grid">
            {testedKeys.map((tested, index) => (
              <div
                className={tested ? "diagnostic-key passed" : "diagnostic-key"}
                key={index}
              >
                <span>{t.keymap.key(index + 1)}</span>
                <strong>{tested ? t.diagnostics.checked : t.diagnostics.unchecked}</strong>
              </div>
            ))}
          </div>
        </section>

        <aside className="panel diagnostics-panel">
          <div className="panel-meta">
            <span className="panel-kicker">{t.diagnostics.functionCheckKicker}</span>
            <h2>{t.diagnostics.functionCheckTitle}</h2>
          </div>
          <dl className="diagnostics-list">
            <div>
              <dt>{t.diagnostics.webHid}</dt>
              <DiagnosticResult passed={typeof navigator !== "undefined" && "hid" in navigator} />
            </div>
            <div>
              <dt>{t.diagnostics.deviceConnection}</dt>
              <DiagnosticResult passed={connected} />
            </div>
            <div>
              <dt>{t.diagnostics.keyEvent}</dt>
              <DiagnosticResult passed={testedCount > 0} />
            </div>
            <div>
              <dt>{t.diagnostics.reportSend}</dt>
              <DiagnosticResult passed={reportTestStatus === "passed"} />
            </div>
            <div>
              <dt>{t.diagnostics.reportDetail}</dt>
              <dd className="diagnostics-detail">{reportTestDetail}</dd>
            </div>
            <div>
              <dt>{t.diagnostics.storageWriteRead}</dt>
              <DiagnosticResult passed={storageTestStatus === "passed"} />
            </div>
            <div>
              <dt>{t.diagnostics.storageDetail}</dt>
              <dd className="diagnostics-detail">{storageTestDetail}</dd>
            </div>
            <div>
              <dt>{t.diagnostics.reportKeys}</dt>
              <dd className="diagnostics-detail">{deviceState?.keyCount ?? "-"}</dd>
            </div>
          </dl>
        </aside>
      </section>
    </main>
  );
}

function DiagnosticResult({ passed }: { passed: boolean }) {
  return (
    <dd className={passed ? "diagnostics-result passed" : "diagnostics-result failed"}>
      {passed ? t.diagnostics.ok : t.diagnostics.ng}
    </dd>
  );
}
