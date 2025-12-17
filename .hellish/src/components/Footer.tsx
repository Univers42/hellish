import React from 'react';
import Link from '@docusaurus/Link';
import './footer.css';

export default function Footer(): JSX.Element {
  const year = new Date().getFullYear();

  return (
    <footer className="hellish-footer" role="contentinfo" aria-label="Site footer">
      <div className="hellish-footer__embers" aria-hidden />

      <div className="hellish-footer__inner container">
        <div className="hellish-footer__grid">
          <div className="footer-col footer-col--brand">
            <div className="brand">
              <svg className="brand__logo" viewBox="0 0 24 24" aria-hidden focusable="false">
                <defs>
                  <linearGradient id="f-lg" x1="0" x2="1">
                    <stop offset="0" stopColor="var(--flame-color-1)" />
                    <stop offset="1" stopColor="var(--flame-color-2)" />
                  </linearGradient>
                </defs>
                <rect x="2" y="4" width="20" height="16" rx="4" fill="url(#f-lg)" />
              </svg>

              <div className="brand__meta">
                <strong className="brand__title">Hellish</strong>
                <p className="brand__tag">Terminal, but alive — fast, minimal, beautiful.</p>
              </div>
            </div>

            <p className="brand__small">
              Lightweight CLI UX with plugins, great docs and a tiny visual soul.
            </p>
          </div>

          <nav className="footer-col" aria-label="Documentation links">
            <h4>Docs</h4>
            <ul className="footer-list">
              <li><Link to="/docs/intro">Introduction</Link></li>
              <li><Link to="/docs/getting-started">Getting started</Link></li>
              <li><Link to="/docs/api">API reference</Link></li>
            </ul>
          </nav>

          <div className="footer-col" aria-label="Community links">
            <h4>Community</h4>
            <ul className="footer-list">
              <li><a href="https://github.com/your-org/hellish" target="_blank" rel="noopener noreferrer">GitHub</a></li>
              <li><a href="https://discord.gg/hellish" target="_blank" rel="noopener noreferrer">Discord</a></li>
              <li><a href="https://twitter.com/hellish" target="_blank" rel="noopener noreferrer">Twitter</a></li>
            </ul>
          </div>

          <div className="footer-col" aria-label="More links">
            <h4>More</h4>
            <ul className="footer-list">
              <li><Link to="/blog">Blog</Link></li>
              <li><a href="/changelog">Changelog</a></li>
              <li><a href="/privacy">Privacy</a></li>
            </ul>

            <div className="footer-socials" aria-hidden>
              <a className="social" href="https://github.com/your-org/hellish" aria-label="GitHub" target="_blank" rel="noopener noreferrer">
                <svg viewBox="0 0 24 24" aria-hidden><path d="M12 0C5.37 0 0 5.37 0 12c0 5.3 3.44 9.8 8.2 11.38.6.11.82-.26.82-.58v-2.23C6.36 20.5 5.67 19.38 5.67 19.38c-.55-1.39-1.33-1.76-1.33-1.76-1.09-.75.08-.73.08-.73 1.2.08 1.84 1.23 1.84 1.23 1.07 1.83 2.81 1.3 3.49 1 .11-.77.43-1.3.76-1.6-2.66-.3-5.47-1.33-5.47-5.93 0-1.31.47-2.38 1.24-3.22-.12-.3-.54-1.52.12-3.17 0 0 1.01-.32 3.31 1.23A11.5 11.5 0 0112 5.8c1.03 0 2.05.14 3.01.41 2.29-1.55 3.3-1.23 3.3-1.23.65 1.65.24 2.87.12 3.17.77.84 1.23 1.91 1.23 3.22 0 4.61-2.81 5.62-5.49 5.92.43.37.82 1.1.82 2.22v3.29c0 .32.19.69.82.57C20.56 21.8 24 17.3 24 12 24 5.37 18.63 0 12 0z"/></svg>
              </a>

              <a className="social" href="https://twitter.com/hellish" aria-label="Twitter" target="_blank" rel="noopener noreferrer">
                <svg viewBox="0 0 24 24" aria-hidden><path d="M23.95 4.57a10 10 0 01-2.82.77 4.95 4.95 0 002.16-2.72c-.95.56-2 1-3.12 1.18a4.93 4.93 0 00-8.39 4.48C7.69 8.1 4.07 6.13 1.64 3.16a4.82 4.82 0 00-.66 2.48c0 1.71.87 3.21 2.19 4.1A4.9 4.9 0 012 9.47v.06a4.92 4.92 0 003.95 4.82c-.46.12-.95.2-1.45.2-.36 0-.71-.03-1.05-.1a4.93 4.93 0 004.6 3.42A9.87 9.87 0 010 19.54a13.95 13.95 0 007.56 2.21c9.05 0 14-7.5 14-14 0-.21 0-.42-.02-.63A10 10 0 0023.95 4.57z"/></svg>
              </a>
            </div>
          </div>
        </div>

        <div className="hellish-footer__bottom" aria-label="Legal">
          <div className="copyright">© {year} Hellish · Built with passion.</div>
          <div className="legal">
            <a href="/terms">Terms</a>
            <span className="sep" aria-hidden>·</span>
            <a href="/privacy">Privacy</a>
          </div>
        </div>
      </div>
    </footer>
  );
}
