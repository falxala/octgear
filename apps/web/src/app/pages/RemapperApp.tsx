import { useEffect, useMemo, useRef, useState } from "react";
import { EditorPanel } from "../components/EditorPanel";
import { FirmwarePanel } from "../components/FirmwarePanel";
import { HardwarePanel } from "../components/HardwarePanel";
import { KeyboardPickerPanel } from "../components/KeyboardPickerPanel";
import { RemapPanel } from "../components/RemapPanel";
import { homeUrl } from "../appUrls";
import { useDeviceSession } from "../hooks/useDeviceSession";
import {
  createModifierMaskFromSlots,
  createModifierSlotsFromMask,
} from "../remapper/modifierSlots";
import {
  applyLayerNavigationOverrides,
  collectChangedAssignments,
  createBlankKeymap,
  expandKeymap,
  normalizeKeymapForDevice,
  updateKeymap,
  updateKeymapKeyAcrossLayers,
} from "../updateKeymap";
import {
  clearDeviceLayerColorPreview,
  enterDeviceBootloader,
  getDeviceState,
  previewDeviceLayerColor,
  readDeviceKeymap,
  readDeviceLayerColors,
  sendRemapperHeartbeat,
  setDeviceLayer,
  setDeviceLayerEnabled,
  setDeviceLayerColor,
  setDeviceKey,
  subscribeDeviceKeyEvents,
  type DeviceState,
  type LayerColor,
} from "../../features/device/deviceCommands";
import { WebHidTransport } from "../../features/device/webHidTransport";
import {
  canInstallUf2FromBrowser,
  downloadFirmwareUf2,
  installFirmwareUf2,
} from "../../features/firmware/firmwareUpdater";
import { HARDWARE_CONFIG } from "../../features/hardware/hardwareConfig";
import {
  type ConsumerKeyOption,
  type KeyboardLayoutMode,
  type KeyPickerOption,
} from "../../features/keymap/keyPickerOptions";
import {
  createBlankAssignment,
  createConsumerAssignment,
  createKeyboardAssignment,
  createLayerCycleAssignment,
  createLayerPreviousAssignment,
  createMomentaryLayerAssignment,
  normalizeAssignment,
  type KeyAssignment,
  type KeyAssignmentKind,
} from "../../features/keymap/keymapTypes";
import { t } from "../../shared/i18n";

const LAYER_COLOR_PREVIEW_DEBOUNCE_MS = 60;

type RemapperAppProps = {
  homeHref?: string;
};

export function RemapperApp({ homeHref = homeUrl }: RemapperAppProps) {
  const transport = useMemo(() => new WebHidTransport(), []);
  const [activeLayer, setActiveLayer] = useState(0);
  const [selectedKey, setSelectedKey] = useState(0);
  const [firmwareModalOpen, setFirmwareModalOpen] = useState(false);
  const [status, setStatus] = useState<string>(t.connection.initialStatus);
  const [firmwareStatus, setFirmwareStatus] = useState<string>(t.firmware.initialStatus);
  const [deviceState, setDeviceState] = useState<DeviceState | null>(null);
  const [readKeymap, setReadKeymap] = useState(createInitialKeymap);
  const [writeKeymap, setWriteKeymap] = useState(createInitialKeymap);
  const [readEnabledLayers, setReadEnabledLayers] = useState(createInitialEnabledLayers);
  const [writeEnabledLayers, setWriteEnabledLayers] = useState(createInitialEnabledLayers);
  const [readLayerColors, setReadLayerColors] = useState(createInitialLayerColors);
  const [writeLayerColors, setWriteLayerColors] = useState(createInitialLayerColors);
  const colorPreviewTimerRef = useRef<number | null>(null);
  const colorPreviewQueueRef = useRef<Promise<void>>(Promise.resolve());
  const [keyboardLayout, setKeyboardLayout] = useState<KeyboardLayoutMode>("jis");
  const selectedAssignment = writeKeymap[activeLayer]?.[selectedKey] ?? normalizeAssignment({ kind: "none" });
  const [draftAssignment, setDraftAssignment] = useState<KeyAssignment>(selectedAssignment);
  const [modifierSlots, setModifierSlots] = useState<number[]>(createModifierSlotsFromMask(0));
  const connected = deviceState !== null && transport.connected;
  const deviceLayerCount = deviceState?.layerCount ?? 0;
  const deviceKeyCount = deviceState?.keyCount ?? 0;
  const firmwareInstallSupported = canInstallUf2FromBrowser();
  const disconnectDevice = useDeviceSession(transport, connected, () => {
    setDeviceState(null);
    setStatus(t.connection.initialStatus);
  });

  useEffect(() => {
    setDraftAssignment(selectedAssignment);
    setModifierSlots(
      createModifierSlotsFromMask(selectedAssignment.kind === "keyboard" ? selectedAssignment.modifier : 0),
    );
  }, [selectedAssignment]);

  useEffect(() => () => cancelScheduledColorPreview(), []);

  useEffect(() => {
    if (!connected || deviceLayerCount === 0 || deviceKeyCount === 0) {
      return;
    }

    return subscribeDeviceKeyEvents(transport, (event) => {
      if (!event.pressed) {
        return;
      }

      if (event.layer >= deviceLayerCount || event.keyIndex >= deviceKeyCount) {
        return;
      }

      setActiveLayer(event.layer);
      setSelectedKey(event.keyIndex);
      setDeviceState((current) =>
        current && current.activeLayer !== event.layer ? { ...current, activeLayer: event.layer } : current,
      );
    });
  }, [connected, deviceKeyCount, deviceLayerCount, transport]);

  async function connectDevice() {
    try {
      const device = await transport.requestDevice();
      await transport.open();
      await sendRemapperHeartbeat(transport);
      const state = await getDeviceState(transport);
      const loadedKeymap = await readDeviceKeymap(transport, state.layerCount, state.keyCount);
      const loadedLayerColors = await readDeviceLayerColors(
        transport,
        state.layerCount,
        HARDWARE_CONFIG.defaultLayerColors,
      );
      console.info("[hid] connected", {
        productName: device.productName,
        vendorId: `0x${device.vendorId.toString(16).padStart(4, "0").toUpperCase()}`,
        productId: `0x${device.productId.toString(16).padStart(4, "0").toUpperCase()}`,
      });
      const uiKeymap = expandKeymap(loadedKeymap, state.layerCount, HARDWARE_CONFIG.keyCount);
      setDeviceState(state);
      setReadKeymap(uiKeymap);
      setWriteKeymap(applyLayerNavigationOverrides(uiKeymap));
      setReadEnabledLayers(state.enabledLayers);
      setWriteEnabledLayers(state.enabledLayers);
      setReadLayerColors(loadedLayerColors);
      setWriteLayerColors(loadedLayerColors);
      setActiveLayer(state.activeLayer);
      setSelectedKey((current) => (current < state.keyCount ? current : 0));
      setStatus(t.connection.connectedTo(device.productName || t.device.fallbackName));
    } catch (error) {
      setStatus(error instanceof Error ? error.message : t.connection.connectFailed);
    }
  }

  async function selectLayer(layerIndex: number) {
    setActiveLayer(layerIndex);
    cancelScheduledColorPreview();

    if (!transport.connected) {
      return;
    }

    try {
      await clearLayerColorPreview();
      if (!writeEnabledLayers[layerIndex]) {
        return;
      }
      await setDeviceLayer(transport, layerIndex);
      setDeviceState((current) =>
        current ? { ...current, activeLayer: layerIndex } : current,
      );
    } catch (error) {
      setStatus(error instanceof Error ? error.message : t.connection.layerChangeFailed);
    }
  }

  async function readAllAssignments() {
    if (!connected || !deviceState) {
      setStatus(t.connection.deviceNotConnected);
      return;
    }

    try {
      setStatus(t.keymap.reading);
      await clearLayerColorPreview();
      const state = await getDeviceState(transport);
      const loadedKeymap = await readDeviceKeymap(transport, state.layerCount, state.keyCount);
      const loadedLayerColors = await readDeviceLayerColors(
        transport,
        state.layerCount,
        HARDWARE_CONFIG.defaultLayerColors,
      );
      const uiKeymap = expandKeymap(loadedKeymap, state.layerCount, HARDWARE_CONFIG.keyCount);
      setDeviceState(state);
      setReadKeymap(uiKeymap);
      setWriteKeymap(applyLayerNavigationOverrides(uiKeymap));
      setReadEnabledLayers(state.enabledLayers);
      setWriteEnabledLayers(state.enabledLayers);
      setReadLayerColors(loadedLayerColors);
      setWriteLayerColors(loadedLayerColors);
      setStatus(t.keymap.readComplete);
    } catch (error) {
      setStatus(error instanceof Error ? error.message : t.keymap.readFailed);
    }
  }

  async function saveAllAssignments() {
    if (!connected || !deviceState) {
      setStatus(t.connection.deviceNotConnected);
      return;
    }

    const keymapToSave = normalizeKeymapForDevice(
      writeKeymap,
      deviceState.layerCount,
      deviceState.keyCount,
    );
    const saveTargets = collectChangedAssignments(readKeymap, keymapToSave);
    const layerTargets = writeEnabledLayers
      .map((enabled, layer) => ({ layer, enabled }))
      .filter(({ layer, enabled }) => layer > 0 && readEnabledLayers[layer] !== enabled);
    const colorTargets = writeLayerColors
      .slice(0, deviceState.layerCount)
      .map((color, layer) => ({ layer, color }))
      .filter(({ layer, color }) => !sameLayerColor(readLayerColors[layer], color));
    try {
      await clearLayerColorPreview();
      if (saveTargets.length === 0 && layerTargets.length === 0 && colorTargets.length === 0) {
        setStatus(t.keymap.saveSkippedAll);
        return;
      }

      setStatus(t.keymap.savingAll);
      let latestDeviceState = deviceState;
      for (const { layer, enabled } of layerTargets) {
        const result = await setDeviceLayerEnabled(
          transport,
          layer,
          enabled,
          deviceState.layerCount,
        );
        latestDeviceState = {
          ...latestDeviceState,
          activeLayer: result.activeLayer,
          enabledLayers: result.enabledLayers,
        };
      }

      for (const { layer, color } of colorTargets) {
        await setDeviceLayerColor(transport, layer, color);
      }

      for (const { layer, keyIndex, assignment } of saveTargets) {
        await setDeviceKey(transport, layer, keyIndex, assignment);
      }

      setDeviceState(latestDeviceState);
      setReadKeymap(keymapToSave);
      setWriteKeymap(keymapToSave);
      setReadEnabledLayers(writeEnabledLayers);
      setReadLayerColors(writeLayerColors);
      setStatus(t.keymap.savedAll(
        saveTargets.length + layerTargets.length + colorTargets.length,
        deviceState.layerCount,
        deviceState.keyCount,
      ));
    } catch (error) {
      setStatus(error instanceof Error ? error.message : t.keymap.saveFailed);
    }
  }

  async function enterBootloaderMode() {
    if (!connected) {
      setFirmwareStatus(t.connection.deviceNotConnected);
      return;
    }

    try {
      setFirmwareStatus(t.firmware.enteringBootloader);
      await enterDeviceBootloader(transport);
      await transport.close().catch(() => undefined);
      setDeviceState(null);
      setStatus(t.firmware.bootMode);
      setFirmwareStatus(t.firmware.bootDriveReady);
    } catch (error) {
      setFirmwareStatus(error instanceof Error ? error.message : t.firmware.enterBootloaderFailed);
    }
  }

  async function installBundledFirmware() {
    try {
      setFirmwareStatus(t.firmware.writing);
      const result = await installFirmwareUf2();
      setFirmwareStatus(t.firmware.written(result.fileName, Math.ceil(result.size / 1024)));
    } catch (error) {
      setFirmwareStatus(error instanceof Error ? error.message : t.firmware.writeFailed);
    }
  }

  async function downloadBundledFirmware() {
    try {
      setFirmwareStatus(t.firmware.downloading);
      await downloadFirmwareUf2();
      setFirmwareStatus(t.firmware.downloaded);
    } catch (error) {
      setFirmwareStatus(error instanceof Error ? error.message : t.firmware.downloadFailed);
    }
  }

  function updateDraftKind(kind: KeyAssignmentKind) {
    updateSelectedAssignment((current) => normalizeAssignment({ ...current, kind }));
    setModifierSlots((current) => (kind === "keyboard" ? current : createModifierSlotsFromMask(0)));
  }

  function updateDraftUsage(usage: number) {
    updateSelectedAssignment((current) =>
      normalizeAssignment({
        ...current,
        usage,
        targetLayer:
          current.kind === "momentaryLayer" ? usage : current.targetLayer,
        keycodes: current.kind === "keyboard" ? [usage, 0, 0, 0, 0, 0] : current.keycodes,
      }),
    );
  }

  function updateDraftModifier(modifier: number) {
    updateSelectedAssignment((current) => {
      const usage = current.kind === "keyboard" ? current.usage : 0;
      const keycodes = current.kind === "keyboard" ? current.keycodes : [0, 0, 0, 0, 0, 0];
      return createKeyboardAssignment(usage, modifier, keycodes);
    });
  }

  function updateDraftModifierSlot(index: number, modifier: number) {
    setModifierSlots((current) => {
      const next = current.map((value, currentIndex) => (currentIndex === index ? modifier : value));
      updateDraftModifier(createModifierMaskFromSlots(next));
      return next;
    });
  }

  function applyPickerOption(option: KeyPickerOption) {
    if (option.kind === "spacer" || option.kind === "decoration") {
      return;
    }

    if (option.kind === "blank") {
      updateSelectedAssignment(() => createBlankAssignment());
      setModifierSlots(createModifierSlotsFromMask(0));
      return;
    }

    if (option.kind === "modifier") {
      updateSelectedAssignment((current) => {
        const modifier = current.kind === "keyboard" ? current.modifier : 0;
        const usage = current.kind === "keyboard" ? current.usage : 0;
        const keycodes = current.kind === "keyboard" ? current.keycodes : [0, 0, 0, 0, 0, 0];
        const nextModifier = modifier ^ option.modifier;
        setModifierSlots(createModifierSlotsFromMask(nextModifier));
        return createKeyboardAssignment(usage, nextModifier, keycodes);
      });
      return;
    }

    updateSelectedAssignment((current) => {
      const modifier = current.kind === "keyboard" ? current.modifier : 0;
      return createKeyboardAssignment(option.code, modifier);
    });
  }

  function applyConsumerOption(option: ConsumerKeyOption) {
    updateSelectedAssignment(() => createConsumerAssignment(option.usage));
  }

  function applyLayerCycleOption() {
    updateSelectedAssignment(() => createLayerCycleAssignment());
  }

  function applyLayerPreviousOption() {
    updateSelectedAssignment(() => createLayerPreviousAssignment());
  }

  function applyMomentaryLayerOption(layer: number) {
    updateSelectedAssignment(() => createMomentaryLayerAssignment(layer));
  }

  function updateSelectedAssignment(updater: (current: KeyAssignment) => KeyAssignment) {
    setDraftAssignment((current) => {
      const next = normalizeAssignment(updater(current));
      const updateAcrossLayers =
        current.kind === "layerCycle" ||
        current.kind === "layerPrevious" ||
        next.kind === "layerCycle" ||
        next.kind === "layerPrevious";
      setWriteKeymap((currentKeymap) =>
        updateAcrossLayers
          ? updateKeymapKeyAcrossLayers(currentKeymap, selectedKey, next)
          : updateKeymap(currentKeymap, activeLayer, selectedKey, next),
      );
      return next;
    });
  }

  function updateLayerEnabled(layer: number, enabled: boolean) {
    if (layer === 0) {
      return;
    }

    setWriteEnabledLayers((current) =>
      current.map((value, index) => (index === layer ? enabled : value)),
    );
  }

  function updateLayerColor(layer: number, color: LayerColor) {
    const normalizedColor = normalizeLayerColor(color);
    setWriteLayerColors((current) =>
      current.map((value, index) => (index === layer ? normalizedColor : value)),
    );

    if (!connected || layer >= deviceLayerCount) {
      return;
    }

    cancelScheduledColorPreview();
    colorPreviewTimerRef.current = window.setTimeout(() => {
      colorPreviewTimerRef.current = null;
      colorPreviewQueueRef.current = colorPreviewQueueRef.current
        .catch(() => undefined)
        .then(() => previewDeviceLayerColor(transport, layer, normalizedColor))
        .catch((error) => {
          setStatus(error instanceof Error ? error.message : t.connection.layerChangeFailed);
        });
    }, LAYER_COLOR_PREVIEW_DEBOUNCE_MS);
  }

  function cancelScheduledColorPreview() {
    if (colorPreviewTimerRef.current !== null) {
      window.clearTimeout(colorPreviewTimerRef.current);
      colorPreviewTimerRef.current = null;
    }
  }

  async function clearLayerColorPreview() {
    cancelScheduledColorPreview();
    const request = colorPreviewQueueRef.current
      .catch(() => undefined)
      .then(() => clearDeviceLayerColorPreview(transport));
    colorPreviewQueueRef.current = request;
    await request;
  }

  async function disconnectRemapperDevice() {
    if (transport.connected) {
      await clearLayerColorPreview().catch(() => undefined);
    }
    await disconnectDevice();
  }

  return (
    <main className="app-shell">
      <header className="topbar">
        <a className="brand brand-link" href={homeHref} aria-label={t.home.backHome}>
          <img src={`${import.meta.env.BASE_URL}cy.png`} alt="" />
          <div className="brand-copy">
            <span className="eyebrow">{t.app.eyebrow}</span>
            <h1>{t.app.title}</h1>
            <p>{t.app.description}</p>
          </div>
        </a>
        <div className="connection">
          <div className="connection-meta">
            <span className={connected ? "status-badge online" : "status-badge offline"}>
              {connected ? t.connection.connected : t.connection.idle}
            </span>
            <span className="connection-text">{status}</span>
          </div>
          <div className="connection-actions">
            <button type="button" className="ghost-button" onClick={() => setFirmwareModalOpen(true)}>
              {t.connection.updater}
            </button>
            <button type="button" onClick={connected ? disconnectRemapperDevice : connectDevice}>
              {connected ? t.connection.disconnect : t.connection.connect}
            </button>
          </div>
        </div>
      </header>

      <section className="workspace" aria-label={t.app.workspaceLabel}>
        <HardwarePanel deviceState={deviceState} />
        <RemapPanel
          activeLayer={activeLayer}
          selectedKey={selectedKey}
          connected={connected}
          supportedKeyCount={connected ? deviceKeyCount : writeKeymap[activeLayer]?.length ?? 0}
          layerCount={writeKeymap.length}
          enabledLayers={writeEnabledLayers}
          layerColors={writeLayerColors}
          layerAssignments={writeKeymap[activeLayer]}
          onRead={() => void readAllAssignments()}
          onSave={() => void saveAllAssignments()}
          onSelectLayer={(layerIndex) => void selectLayer(layerIndex)}
          onLayerEnabledChange={updateLayerEnabled}
          onLayerColorChange={updateLayerColor}
          onSelectKey={setSelectedKey}
        />
        <EditorPanel
          selectedKey={selectedKey}
          draftAssignment={draftAssignment}
          onUpdateKind={updateDraftKind}
          onUpdateUsage={updateDraftUsage}
          modifierSlots={modifierSlots}
          onUpdateModifierSlot={updateDraftModifierSlot}
        />
        <KeyboardPickerPanel
          draftAssignment={draftAssignment}
          keyboardLayout={keyboardLayout}
          onKeyboardLayoutChange={setKeyboardLayout}
          onPickerOption={applyPickerOption}
          onConsumerOption={applyConsumerOption}
          onLayerCycleOption={applyLayerCycleOption}
          onLayerPreviousOption={applyLayerPreviousOption}
          onMomentaryLayerOption={applyMomentaryLayerOption}
        />
      </section>

      {firmwareModalOpen ? (
        <div
          className="modal-backdrop"
          role="presentation"
          onClick={() => setFirmwareModalOpen(false)}
        >
          <div
            className="modal-shell"
            role="dialog"
            aria-modal="true"
            aria-labelledby="firmware-modal-title"
            onClick={(event) => event.stopPropagation()}
          >
            <div className="modal-header">
              <div className="panel-meta">
                <span className="panel-kicker">{t.firmware.updater}</span>
                <h2 id="firmware-modal-title">{t.firmware.title}</h2>
              </div>
              <button
                type="button"
                className="ghost-button"
                onClick={() => setFirmwareModalOpen(false)}
                aria-label={t.firmware.closeLabel}
              >
                {t.firmware.close}
              </button>
            </div>
            <FirmwarePanel
              connected={connected}
              firmwareInstallSupported={firmwareInstallSupported}
              firmwareStatus={firmwareStatus}
              onEnterBootloader={() => void enterBootloaderMode()}
              onInstallFirmware={() => void installBundledFirmware()}
              onDownloadFirmware={() => void downloadBundledFirmware()}
            />
          </div>
        </div>
      ) : null}
    </main>
  );
}

function createInitialKeymap() {
  return createBlankKeymap(HARDWARE_CONFIG.layerCount, HARDWARE_CONFIG.keyCount);
}

function createInitialEnabledLayers() {
  return Array.from(
    { length: HARDWARE_CONFIG.layerCount },
    (_, layer) => HARDWARE_CONFIG.defaultEnabledLayers.includes(layer),
  );
}

function createInitialLayerColors(): LayerColor[] {
  return HARDWARE_CONFIG.defaultLayerColors.map(([red, green, blue]) => ({ red, green, blue }));
}

function normalizeLayerColor(color: LayerColor): LayerColor {
  return {
    red: clampColorChannel(color.red),
    green: clampColorChannel(color.green),
    blue: clampColorChannel(color.blue),
  };
}

function sameLayerColor(first: LayerColor | undefined, second: LayerColor) {
  return first?.red === second.red && first.green === second.green && first.blue === second.blue;
}

function clampColorChannel(value: number) {
  return Math.max(0, Math.min(255, Math.trunc(value)));
}
