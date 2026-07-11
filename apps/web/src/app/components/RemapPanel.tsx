import { ConsumerKeycapSvg } from "./ConsumerKeycapSvg";
import { HARDWARE_CONFIG } from "../../features/hardware/hardwareConfig";
import { consumerOptionByUsage } from "../../features/keymap/keyPickerOptions";
import type { KeyAssignment } from "../../features/keymap/keymapTypes";
import { t } from "../../shared/i18n";

type RemapPanelProps = {
  activeLayer: number;
  selectedKey: number;
  connected: boolean;
  supportedKeyCount: number;
  layerCount: number;
  enabledLayers: boolean[];
  layerAssignments: KeyAssignment[];
  onRead: () => void;
  onSave: () => void;
  onSelectLayer: (layerIndex: number) => void;
  onLayerEnabledChange: (layerIndex: number, enabled: boolean) => void;
  onSelectKey: (keyIndex: number) => void;
};

export function RemapPanel({
  activeLayer,
  selectedKey,
  connected,
  supportedKeyCount,
  layerCount,
  enabledLayers,
  layerAssignments,
  onRead,
  onSave,
  onSelectLayer,
  onLayerEnabledChange,
  onSelectKey,
}: RemapPanelProps) {
  const controls = HARDWARE_CONFIG.controls.slice(0, layerAssignments.length);
  const keyControls = controls.filter((control) => control.kind === "key");
  const encoderControls = controls.filter((control) => control.kind !== "key");

  return (
    <section className="panel remap-panel">
      <div className="panel-heading">
        <div className="panel-meta">
          <span className="panel-kicker">{t.keymap.kicker}</span>
          <h2>{t.keymap.title}</h2>
        </div>
        <div className="remap-actions">
          <button type="button" onClick={onRead} disabled={!connected}>
            {t.keymap.read}
          </button>
          <button type="button" className="primary-action" onClick={onSave} disabled={!connected}>
            {t.keymap.save}
          </button>
        </div>
      </div>

      <div className="remap-strip">
        <span className="strip-label">{t.keymap.layer}</span>
        <div className="layer-tabs" aria-label={t.keymap.layer}>
          {Array.from({ length: layerCount }, (_, layerIndex) => (
            <div
              key={layerIndex}
              className={`layer-tab-item ${enabledLayers[layerIndex] ? "enabled" : "disabled"}`}
            >
              <button
                type="button"
                className={layerIndex === activeLayer ? "active" : ""}
                onClick={() => onSelectLayer(layerIndex)}
              >
                {layerIndex}
              </button>
              <label className="layer-enabled-toggle">
                <input
                  type="checkbox"
                  checked={enabledLayers[layerIndex] ?? true}
                  disabled={layerIndex === 0}
                  aria-label={t.keymap.layerEnabledLabel(layerIndex)}
                  title={layerIndex === 0 ? t.keymap.baseLayerRequired : undefined}
                  onChange={(event) => onLayerEnabledChange(layerIndex, event.target.checked)}
                />
                <span>{enabledLayers[layerIndex] ? t.keymap.enabled : t.keymap.disabled}</span>
              </label>
            </div>
          ))}
        </div>
      </div>

      <div className="remap-strip keymap-strip">
        <span className="strip-label">{t.keymap.keys}</span>
        <div className="control-map">
          <div className="key-grid" aria-label={t.keymap.keys}>
            {keyControls.map((control) =>
              renderControlTile({
                assignment: layerAssignments[control.bit],
                connected,
                control,
                isSelected: control.bit === selectedKey,
                onSelectKey,
                supported: control.bit < supportedKeyCount,
              }),
            )}
          </div>

          {encoderControls.length > 0 ? (
            <div className="encoder-block">
              <span className="encoder-block-label">{t.keymap.encoder}</span>
              <div className="encoder-grid" aria-label={t.keymap.encoder}>
                {encoderControls.map((control) =>
                  renderControlTile({
                    assignment: layerAssignments[control.bit],
                    connected,
                    control,
                    isSelected: control.bit === selectedKey,
                    onSelectKey,
                    supported: control.bit < supportedKeyCount,
                  }),
                )}
              </div>
            </div>
          ) : null}
        </div>
      </div>
    </section>
  );
}

function renderControlTile({
  assignment,
  connected,
  control,
  isSelected,
  onSelectKey,
  supported,
}: {
  assignment: KeyAssignment;
  connected: boolean;
  control: typeof HARDWARE_CONFIG.controls[number];
  isSelected: boolean;
  onSelectKey: (keyIndex: number) => void;
  supported: boolean;
}) {
  const unsupported = connected && !supported;
  const className = [
    "key-tile",
    control.kind !== "key" ? "encoder-tile" : "",
    control.kind === "encoderSwitch" ? "encoder-switch-tile" : "",
    isSelected ? "active" : "",
    unsupported ? "unsupported" : "",
  ].filter(Boolean).join(" ");

  return (
    <button
      key={control.id}
      type="button"
      className={className}
      disabled={unsupported}
      onClick={() => onSelectKey(control.bit)}
    >
      <span>{control.label}</span>
      <div className="key-tile-assignments">
        <AssignmentPreview assignment={assignment} />
      </div>
    </button>
  );
}

function AssignmentPreview({
  assignment,
}: {
  assignment: KeyAssignment;
}) {
  const consumerOption =
    assignment.kind === "consumer" ? consumerOptionByUsage(assignment.usage) : undefined;

  return (
    <div className="assignment-preview">
      {consumerOption ? (
        <ConsumerKeycapSvg icon={consumerOption.icon} label={assignment.label} variant="tile" />
      ) : (
        <strong>{assignment.kind === "none" ? t.keymap.noAssignment : assignment.label}</strong>
      )}
    </div>
  );
}
