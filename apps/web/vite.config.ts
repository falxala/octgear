import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

export default defineConfig(({ mode }) => ({
  base: mode === "github-pages" ? "/octgear/" : "/",
  build: {
    rollupOptions: {
      input: {
        index: "index.html",
        remapper: "remapper.html",
        diagnostics: "diagnostics.html",
      },
    },
  },
  plugins: [react()],
}));
