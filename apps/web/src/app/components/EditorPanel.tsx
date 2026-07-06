import { HARDWARE_CONFIG } from "../../features/hardware/hardwareConfig";
import { modifierOptions } from "../../features/keymap/keyPickerOptions";
import {
  formatHex,
  type KeyAssignment,
  type KeyAssignmentKind,
} from "../../features/keymap/keymapTypes";
import { t } from "../../shared/i18n";

type EditorPanelProps = {
  selectedKey: number;
  draftAssignment: KeyAssignment;
  onUpdateKind: (kind: KeyAssignmentKind) => void;
  onUpdateUsage: (usage: number) => void;
  modifierSlots: number[];
  onUpdateModifierSlot: (index: number, modifier: number) => void;
};

export function EditorPanel({
  selectedKey,
  draftAssignment,
  onUpdateKind,
  onUpdateUsage,
  modifierSlots,
  onUpdateModifierSlot,
}: EditorPanelProps) {
  let usageMax = 255;
  if (draftAssignment.kind === "consumer") {
    usageMax = 65535;
  } else if (draftAssignment.kind === "momentaryLayer") {
    usageMax = HARDWARE_CONFIG.layerCount - 1;
  }

  const usesTargetLayer = draftAssignment.kind === "momentaryLayer";
  const usageLabel = usesTargetLayer ? t.assignment.targetLayer : t.assignment.usage;
  const valueLabel = usesTargetLayer ? t.assignment.targetLayer : t.assignment.usageHex;
  const assignmentValue = usesTargetLayer ? draftAssignment.targetLayer : draftAssignment.usage;
  const valueDisabled = draftAssignment.kind === "none" || draftAssignment.kind === "layerCycle";
  let summaryValue = formatHex(draftAssignment.usage, draftAssignment.kind === "consumer" ? 4 : 2);
  if (draftAssignment.kind === "layerCycle") {
    summaryValue = "-";
  } else if (usesTargetLayer) {
    summaryValue = String(draftAssignment.targetLayer);
  }

  return (
    <aside className="panel editor-panel">
      <div className="panel-heading compact">
        <div className="panel-meta">
          <span className="panel-kicker">{t.assignment.kicker}</span>
          <h2>{t.keymap.key(selectedKey + 1)}</h2>
        </div>
      </div>

      <label>
        <span>{t.assignment.type}</span>
        <select
          value={draftAssignment.kind}
          onChange={(event) => onUpdateKind(event.currentTarget.value as KeyAssignmentKind)}
        >
          <option value="none">{t.assignment.none}</option>
          <option value="keyboard">{t.assignment.keyboard}</option>
          <option value="consumer">{t.assignment.consumer}</option>
          <option value="layerCycle">{t.assignment.layerCycle}</option>
          <option value="momentaryLayer">{t.assignment.momentaryLayer}</option>
        </select>
      </label>

      <label>
        <span>{usageLabel}</span>
        <input
          type="number"
          min={0}
          max={usageMax}
          value={assignmentValue}
          disabled={valueDisabled}
          onChange={(event) => onUpdateUsage(Number(event.currentTarget.value))}
        />
      </label>

      <label className="modifier-field">
        <span>{t.assignment.modifier}</span>
        <div className="modifier-selects">
          {modifierSlots.map((value, index) => (
            <select
              key={index}
              value={value}
              disabled={draftAssignment.kind !== "keyboard"}
              onChange={(event) => onUpdateModifierSlot(index, Number(event.currentTarget.value))}
            >
              <option value={0}>{t.assignment.none}</option>
              {modifierOptions.map((option) => (
                <option
                  key={option.modifier}
                  value={option.modifier}
                  disabled={
                    value !== option.modifier &&
                    modifierSlots.some((selectedModifier) => selectedModifier === option.modifier)
                  }
                >
                  {option.label}
                </option>
              ))}
            </select>
          ))}
        </div>
      </label>

      <dl className="assignment-summary">
        <div>
          <dt>{t.assignment.label}</dt>
          <dd>{draftAssignment.label}</dd>
        </div>
        <div>
          <dt>{valueLabel}</dt>
          <dd>{summaryValue}</dd>
        </div>
      </dl>
    </aside>
  );
}
