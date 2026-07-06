import { StrictMode } from "react";
import { createRoot } from "react-dom/client";
import { DiagnosticsApp } from "../app/pages/DiagnosticsApp";
import "../app/styles.css";

createRoot(document.getElementById("root")!).render(
  <StrictMode>
    <DiagnosticsApp />
  </StrictMode>,
);
