import { en } from "./en";
import { ja } from "./ja";

export type Locale = "ja" | "en";
export type Messages = typeof ja;

export const messages = {
  ja,
  en,
} satisfies Record<Locale, Messages>;

export const DEFAULT_LOCALE: Locale = "ja";
export const t = messages[DEFAULT_LOCALE];
