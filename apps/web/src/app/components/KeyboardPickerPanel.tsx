import type { CSSProperties } from "react";

import { ConsumerKeycapSvg } from "./ConsumerKeycapSvg";
import { KeycapSvg } from "./KeycapSvg";
import { HARDWARE_CONFIG } from "../../features/hardware/hardwareConfig";
import {
  blankOption,
  consumerOptions,
  keyboardRowsForLayout,
  keyOptionLabel,
  navigationRows,
  numpadRows,
  type ConsumerKeyOption,
  type KeyboardLayoutMode,
  type KeyPickerOption,
} from "../../features/keymap/keyPickerOptions";
import type { KeyAssignment } from "../../features/keymap/keymapTypes";
import { t } from "../../shared/i18n";

type KeyboardPickerPanelProps = {
  draftAssignment: KeyAssignment;
  keyboardLayout: KeyboardLayoutMode;
  onKeyboardLayoutChange: (layout: KeyboardLayoutMode) => void;
  onPickerOption: (option: KeyPickerOption) => void;
  onConsumerOption: (option: ConsumerKeyOption) => void;
  onLayerCycleOption: () => void;
  onLayerPreviousOption: () => void;
  onMomentaryLayerOption: (layer: number) => void;
};

export function KeyboardPickerPanel({
  draftAssignment,
  keyboardLayout,
  onKeyboardLayoutChange,
  onPickerOption,
  onConsumerOption,
  onLayerCycleOption,
  onLayerPreviousOption,
  onMomentaryLayerOption,
}: KeyboardPickerPanelProps) {
  const systemRows = navigationRows.slice(0, 1);
  const navigationBodyRows = navigationRows.slice(1);
  const keyboardRows = keyboardRowsForLayout(keyboardLayout);

  function pickerOptionClassName(option: KeyPickerOption) {
    if (option.kind === "spacer") {
      return "picker-spacer";
    }

    if (option.kind === "decoration") {
      return "picker-decor";
    }

    const accent = option.accent ? " layout-accent" : "";
    const active =
      (option.kind === "blank" && draftAssignment.kind === "none") ||
      (option.kind === "key" &&
        draftAssignment.kind === "keyboard" &&
        draftAssignment.usage === option.code) ||
      (option.kind === "modifier" &&
        draftAssignment.kind === "keyboard" &&
        (draftAssignment.modifier & option.modifier) !== 0);

    return active ? `picker-key active${accent}` : `picker-key${accent}`;
  }

  function keyShape(option: KeyPickerOption) {
    if (option.kind === "key" && option.code === 40 && keyboardLayout === "jis") {
      return "jis-enter" as const;
    }

    if (option.kind === "key" && option.code === 87) {
      return "numpad-plus" as const;
    }

    if (option.kind === "key" && option.code === 88) {
      return "numpad-enter" as const;
    }

    return "regular" as const;
  }

  function renderPickerOption(option: KeyPickerOption, key: string) {
    const width = option.kind === "spacer" ? option.width : option.width ?? 1;
    const style = { "--key-units": width } as CSSProperties;

    if (option.kind === "spacer") {
      return <span key={key} className="picker-spacer" style={style} />;
    }

    if (option.kind === "decoration") {
      if (option.decoration === "jis-enter-lower" && keyboardLayout !== "jis") {
        return null;
      }

      return <span key={key} className={`picker-decor ${option.decoration}`} style={style} aria-hidden="true" />;
    }

    const className = pickerOptionClassName(option);
    const label = keyOptionLabel(option, keyboardLayout);
    const shape = keyShape(option);

    return (
      <button
        key={key}
        type="button"
        className={className}
        style={style}
        onClick={() => onPickerOption(option)}
      >
        <KeycapSvg label={label} units={width} shape={shape} />
      </button>
    );
  }

  return (
    <section className="panel picker-panel">
      <div className="panel-heading">
        <div className="panel-meta">
          <span className="panel-kicker">{t.palette.kicker}</span>
          <h2>{t.palette.title}</h2>
        </div>
        <div className="layout-tabs" aria-label={t.palette.layoutLabel}>
          <button
            type="button"
            className={keyboardLayout === "jis" ? "active" : ""}
            onClick={() => onKeyboardLayoutChange("jis")}
          >
            JIS
          </button>
          <button
            type="button"
            className={keyboardLayout === "us" ? "active" : ""}
            onClick={() => onKeyboardLayoutChange("us")}
          >
            US
          </button>
        </div>
      </div>

      <div className="layer-board">
        <div className="layer-strip">
          <button
            type="button"
            className={draftAssignment.kind === "layerCycle" ? "picker-key active" : "picker-key"}
            onClick={onLayerCycleOption}
          >
            <KeycapSvg label={t.assignment.layerCycleLabel} units={2.16} />
          </button>
          <button
            type="button"
            className={draftAssignment.kind === "layerPrevious" ? "picker-key active" : "picker-key"}
            onClick={onLayerPreviousOption}
          >
            <KeycapSvg label={t.assignment.layerPreviousLabel} units={2.16} />
          </button>
          {Array.from({ length: HARDWARE_CONFIG.layerCount }, (_, layer) => (
            <button
              key={layer}
              type="button"
              className={
                draftAssignment.kind === "momentaryLayer" && draftAssignment.targetLayer === layer
                  ? "picker-key active"
                  : "picker-key"
              }
              onClick={() => onMomentaryLayerOption(layer)}
            >
              <KeycapSvg label={t.assignment.momentaryLayerLabel(layer)} units={2.16} />
            </button>
          ))}
        </div>
      </div>

      <div className="consumer-board">
        <div className="consumer-strip">
          {consumerOptions.map((option) => (
            <button
              key={option.usage}
              type="button"
              className={
                draftAssignment.kind === "consumer" && draftAssignment.usage === option.usage
                  ? "picker-key active"
                  : "picker-key"
              }
              aria-label={option.label}
              onClick={() => onConsumerOption(option)}
            >
              <ConsumerKeycapSvg icon={option.icon} label={option.label} />
            </button>
          ))}
          {renderPickerOption(blankOption, "blank")}
        </div>
      </div>

      <div className="keyboard-scroll">
        <div className={`keyboard-picker ${keyboardLayout}`}>
          <div className="keyboard-main">
            {keyboardRows.map((row, rowIndex) => (
              <div key={rowIndex} className="picker-row">
                {row.map((option, optionIndex) => renderPickerOption(option, `main-${rowIndex}-${optionIndex}`))}
              </div>
            ))}
          </div>

          <div className="keyboard-section navigation-section">
            <div className="keyboard-cluster system-cluster">
              {systemRows.map((row, rowIndex) => (
                <div key={rowIndex} className="picker-row compact">
                  {row.map((option, optionIndex) => renderPickerOption(option, `sys-${rowIndex}-${optionIndex}`))}
                </div>
              ))}
            </div>

            <div className="keyboard-cluster navigation-cluster">
              {navigationBodyRows.map((row, rowIndex) => (
                <div key={rowIndex} className="picker-row compact">
                  {row.map((option, optionIndex) => renderPickerOption(option, `nav-${rowIndex}-${optionIndex}`))}
                </div>
              ))}
            </div>
          </div>

          <div className="keyboard-cluster numpad-cluster">
            {numpadRows.map((row, rowIndex) => (
              <div key={rowIndex} className="picker-row compact">
                {row.map((option, optionIndex) => renderPickerOption(option, `num-${rowIndex}-${optionIndex}`))}
              </div>
            ))}
          </div>
        </div>
      </div>
    </section>
  );
}
