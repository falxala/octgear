import { StrictMode } from "react";
import { createRoot } from "react-dom/client";
import { RemapperApp } from "../app/pages/RemapperApp";
import "../app/styles.css";

createRoot(document.getElementById("root")!).render(
  <StrictMode>
    <RemapperApp />
  </StrictMode>,
);
