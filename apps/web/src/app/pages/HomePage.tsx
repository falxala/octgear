import { diagnosticsUrl, remapperUrl } from "../appUrls";
import { HARDWARE_CONFIG } from "../../features/hardware/hardwareConfig";
import { t } from "../../shared/i18n";

export function HomePage() {
  return (
    <main className="app-shell home-shell">
      <section className="home-hero" aria-labelledby="home-title">
        <div className="brand home-brand">
          <img src={`${import.meta.env.BASE_URL}cy.png`} alt="" />
          <div className="brand-copy">
            <span className="eyebrow">{t.home.eyebrow}</span>
            <h1 id="home-title">{t.home.title}</h1>
            <p>{t.home.description}</p>
          </div>
        </div>
      </section>

      <section className="product-grid" aria-label={t.home.productListLabel}>
        {t.home.products.map((product) => (
          <article className="product-card" key={product.name}>
            <div className="product-card-copy">
              <span className="eyebrow">{product.status}</span>
              <h2>{product.name}</h2>
              <p>{product.description}</p>
            </div>
            <dl className="product-specs">
              <div>
                <dt>{t.home.keys}</dt>
                <dd>{HARDWARE_CONFIG.physicalKeyCount}</dd>
              </div>
              <div>
                <dt>{t.home.encoder}</dt>
                <dd>{t.home.encoderValue}</dd>
              </div>
              <div>
                <dt>{t.home.layers}</dt>
                <dd>{HARDWARE_CONFIG.layerCount}</dd>
              </div>
              <div>
                <dt>{t.home.connection}</dt>
                <dd>{t.home.connectionValue}</dd>
              </div>
            </dl>
            <div className="product-actions">
              <a className="product-action" href={remapperUrl}>
                {t.home.openRemapper}
              </a>
              <a className="product-action secondary" href={diagnosticsUrl}>
                {t.home.openDiagnostics}
              </a>
            </div>
          </article>
        ))}
      </section>
    </main>
  );
}
