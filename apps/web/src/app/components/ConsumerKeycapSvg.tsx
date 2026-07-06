import type { ConsumerKeyOption } from "../../features/keymap/keyPickerOptions";

type ConsumerKeycapSvgProps = {
  icon: ConsumerKeyOption["icon"];
  label: string;
  variant?: "keycap" | "tile";
};

const WIDTH = 95;
const HEIGHT = 44;
const SHADOW_TOP = HEIGHT - 6;

export function ConsumerKeycapSvg({ icon, label, variant = "keycap" }: ConsumerKeycapSvgProps) {
  const className =
    variant === "tile"
      ? "keycap-svg consumer-keycap-svg consumer-keycap-svg-tile"
      : "keycap-svg keycap-svg-regular consumer-keycap-svg";
  const scale = variant === "tile" ? 0.58 : 0.72;

  return (
    <svg className={className} viewBox={`0 0 ${WIDTH} ${HEIGHT}`} role="img">
      <title>{label}</title>
      <rect className="shape-fill" x="1" y="1" width={WIDTH - 2} height={HEIGHT - 2} rx="8" />
      <path className="shape-shadow" d={`M2 ${SHADOW_TOP}H${WIDTH - 2}V${HEIGHT - 2}H2V${SHADOW_TOP}Z`} />
      <rect className="shape-stroke" x="1" y="1" width={WIDTH - 2} height={HEIGHT - 2} rx="8" />
      <g className="consumer-icon" transform={`translate(47.5 22) scale(${scale})`}>
        <ConsumerIcon icon={icon} />
      </g>
    </svg>
  );
}

function ConsumerIcon({ icon }: Pick<ConsumerKeycapSvgProps, "icon">) {
  switch (icon) {
    case "mute":
      return (
        <>
          <SpeakerGlyph />
          <path className="icon-stroke danger" d="M9 -8L23 8M23 -8L9 8" />
        </>
      );
    case "volume-down":
      return (
        <>
          <SpeakerGlyph />
          <path className="icon-stroke" d="M10 -6C14 -2 14 2 10 6" />
        </>
      );
    case "volume-up":
      return (
        <>
          <SpeakerGlyph />
          <path className="icon-stroke" d="M9 -7C13 -3 13 3 9 7M15 -11C22 -4 22 4 15 11" />
        </>
      );
    case "previous":
      return (
        <>
          <path className="icon-fill" d="M-19 -10H-14V10H-19Z" />
          <path className="icon-fill" d="M-12 0L3 -10V10Z" />
          <path className="icon-fill" d="M4 0L19 -10V10Z" />
        </>
      );
    case "play-pause":
      return (
        <>
          <path className="icon-fill" d="M-18 -12L2 0L-18 12Z" />
          <path className="icon-fill" d="M8 -11H13V11H8ZM17 -11H22V11H17Z" />
        </>
      );
    case "next":
      return (
        <>
          <path className="icon-fill" d="M-19 -10L-4 0L-19 10Z" />
          <path className="icon-fill" d="M-3 -10L12 0L-3 10Z" />
          <path className="icon-fill" d="M14 -10H19V10H14Z" />
        </>
      );
    case "brightness-down":
      return <BrightnessGlyph rays="short" />;
    case "brightness-up":
      return <BrightnessGlyph rays="long" />;
  }
}

function SpeakerGlyph() {
  return (
    <>
      <path className="icon-fill" d="M-23 -7H-14L-4 -16V16L-14 7H-23Z" />
      <path className="icon-stroke" d="M-23 -7H-14L-4 -16V16L-14 7H-23Z" />
    </>
  );
}

function BrightnessGlyph({ rays }: { rays: "short" | "long" }) {
  const rayEnd = rays === "long" ? 19 : 15;

  return (
    <>
      <circle className="icon-stroke" cx="0" cy="0" r="7" />
      <path
        className="icon-stroke"
        d={`M0 -${rayEnd}V-13M0 13V${rayEnd}M-${rayEnd} 0H-13M13 0H${rayEnd}M-${rayEnd - 2} -${rayEnd - 2}L-10 -10M10 10L${rayEnd - 2} ${rayEnd - 2}M${rayEnd - 2} -${rayEnd - 2}L10 -10M-10 10L-${rayEnd - 2} ${rayEnd - 2}`}
      />
      {rays === "long" ? null : <circle className="icon-fill" cx="0" cy="0" r="3" />}
    </>
  );
}
