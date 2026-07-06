type KeycapShape = "regular" | "jis-enter" | "numpad-enter" | "numpad-plus";

type KeycapSvgProps = {
  label: string;
  units: number;
  shape?: KeycapShape;
};

const KEY_UNIT = 44;
const KEY_HEIGHT = 44;
const KEY_GAP = 6;

export function KeycapSvg({ label, units, shape = "regular" }: KeycapSvgProps) {
  if (shape === "jis-enter") {
    const textClassName = keycapLabelClassName(label);

    return (
      <svg className="keycap-svg keycap-svg-jis-enter" viewBox="0 0 78 94" aria-hidden="true">
        <path className="shape-fill" d="M1 1H77V93H29V45H1V1Z" />
        <path className="shape-shadow" d="M2 40H26V88H74V92H30V44H2V40Z" />
        <path className="shape-stroke" d="M1 1H77V93H29V45H1V1Z" />
        <text x="39" y="25" textAnchor="middle" className={textClassName}>
          {label}
        </text>
      </svg>
    );
  }

  if (shape === "numpad-enter") {
    const textClassName = keycapLabelClassName(label);

    return (
      <svg className="keycap-svg keycap-svg-numpad-enter" viewBox="0 0 44 94" aria-hidden="true">
        <path className="shape-fill" d="M1 1H43V93H1V1Z" />
        <path className="shape-shadow" d="M2 88H42V92H2V88Z" />
        <path className="shape-stroke" d="M1 1H43V93H1V1Z" />
        <text x="22" y="47" textAnchor="middle" className={textClassName}>
          {label}
        </text>
      </svg>
    );
  }

  if (shape === "numpad-plus") {
    const textClassName = keycapLabelClassName(label);

    return (
      <svg className="keycap-svg keycap-svg-numpad-plus" viewBox="0 0 44 94" aria-hidden="true">
        <path className="shape-fill" d="M1 1H43V93H1V1Z" />
        <path className="shape-shadow" d="M2 88H42V92H2V88Z" />
        <path className="shape-stroke" d="M1 1H43V93H1V1Z" />
        <text x="22" y="47" textAnchor="middle" className={textClassName}>
          {label}
        </text>
      </svg>
    );
  }

  const width = Math.round(KEY_UNIT * units);
  const height = KEY_HEIGHT;
  const shadowTop = height - 6;
  const textClassName = keycapLabelClassName(label);

  return (
    <svg className="keycap-svg keycap-svg-regular" viewBox={`0 0 ${width} ${height}`} aria-hidden="true">
      <rect className="shape-fill" x="1" y="1" width={width - 2} height={height - 2} rx="8" />
      <path className="shape-shadow" d={`M2 ${shadowTop}H${width - 2}V${height - 2}H2V${shadowTop}Z`} />
      <rect className="shape-stroke" x="1" y="1" width={width - 2} height={height - 2} rx="8" />
      <text x={width / 2} y={height / 2 + 2} textAnchor="middle" className={textClassName}>
        {label}
      </text>
    </svg>
  );
}

export function keycapHeightForShape(shape: KeycapShape) {
  if (shape === "regular") {
    return KEY_HEIGHT;
  }

  return (KEY_HEIGHT * 2) + KEY_GAP;
}

function keycapLabelClassName(label: string) {
  if (label.length >= 6) {
    return "shape-label shape-label-compact";
  }

  if (label.length >= 4) {
    return "shape-label shape-label-narrow";
  }

  return "shape-label";
}
