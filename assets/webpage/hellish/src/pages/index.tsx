import React, {useCallback, useEffect, useRef, useState} from 'react';
import Layout from '@theme/Layout';
import Link from '@docusaurus/Link';
import clsx from 'clsx';
import './index.css';
import Footer from '../components/Footer';
import ColorModeToggle from '../components/ColorModeToggle';
import GlitchTitle from '../components/GlitchTitle';

/* ---------- Types ---------- */
type Card = {
  id: string;
  title: string;
  description: string;
  actions: {label: string; kind: 'run' | 'copy' | 'link'; href?: string}[];
};

/* ---------- Sample data ---------- */
const SAMPLE_SNIPPET = `# sample sh42 command
sh42 -v
sh42 run example.sh
`;

const CARDS: Card[] = [
  {
    id: 'cli',
    title: 'Interactive CLI',
    description: 'Try commands, examples and get instant snippets for sh42.',
    actions: [
      {label: 'Run example', kind: 'run'},
      {label: 'Copy snippet', kind: 'copy'},
    ],
  },
  {
    id: 'extensible',
    title: 'Extensible',
    description: 'Plugins, hooks and rich configuration for power users.',
    actions: [{label: 'Learn more', kind: 'link', href: '/docs/intro'}],
  },
  {
    id: 'docs-first',
    title: 'Docs-first',
    description: 'Complete guides: Quick start — Full API — CLI usage — Examples.',
    actions: [{label: 'Get started', kind: 'link', href: '/docs/getting-started'}],
  },
];

/* ---------- Particle Canvas (embers) ---------- */
function useEmberCanvas(enabled = true) {
  const ref = useRef<HTMLCanvasElement | null>(null);

  useEffect(() => {
    if (!enabled) return;
    const canvas = ref.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    // additive blending for soft glows
    ctx.globalCompositeOperation = 'lighter';

    const DPR = Math.max(1, window.devicePixelRatio || 1);
    let w = Math.max(300, canvas.clientWidth || window.innerWidth);
    let h = Math.max(200, canvas.clientHeight || window.innerHeight);
    canvas.width = Math.floor(w * DPR);
    canvas.height = Math.floor(h * DPR);
    ctx.setTransform(DPR, 0, 0, DPR, 0, 0);

    // Parallax / input
    let targetMouseX = w / 2;
    let targetMouseY = h / 2;
    let smoothedMouseX = w / 2;
    let smoothedMouseY = h / 2;
    let targetScrollY = window.scrollY || 0;
    let smoothedScrollY = targetScrollY;

    // particles with depth
    const embers: {x:number;y:number;vx:number;vy:number;size:number;alpha:number;hue:number;depth:number}[] = [];
    const MAX = Math.max(10, Math.floor((w * h) / 30000));

    const spawn = (fromTop = false, depthOverride?: number) => {
      if (embers.length < MAX * 2) {
        const depth = typeof depthOverride === 'number' ? depthOverride : 0.2 + Math.random() * 0.8;
        const size = (0.8 + Math.random() * 3.2) * (1 + (1 - depth) * 0.9);
        const isDark = document.documentElement.getAttribute('data-theme') === 'dark';
        const hue = isDark ? 200 + Math.random() * 40 : 24 + Math.random() * 20;
        embers.push({
          x: Math.random() * w,
          y: fromTop ? -20 : h + Math.random() * 30,
          vx: (Math.random() - 0.5) * (0.3 + (1 - depth) * 0.8),
          vy: - (0.6 + Math.random() * 1.1) * (0.6 + (1 - depth) * 0.9),
          size,
          alpha: 0.08 + Math.random() * 0.42,
          hue,
          depth,
        });
      }
    };

    // seed
    for (let i = 0; i < MAX * 0.6; i++) spawn(true);

    // LIGHTNING / THUNDER VISUALS
    let nextLightningAt = performance.now() + 4000 + Math.random() * 14000; // schedule first strike
    let lightningActive = false;
    let lightningLife = 0; // frames remaining
    let flashAlpha = 0;
    type LightningSegment = {points:{x:number;y:number}[]; width:number; hue:number; alpha:number};
    let lightningSegments: LightningSegment[] = [];

    const triggerLightning = () => {
      // create 1-2 jagged segments across the canvas
      lightningSegments = [];
      const count = 1 + Math.floor(Math.random() * 2);
      for (let s = 0; s < count; s++) {
        const startX = Math.random() * w * 0.6 + w * 0.1;
        const startY = Math.random() * h * 0.3;
        const endX = Math.random() * w * 0.8 + w * 0.1;
        const endY = Math.random() * h * 0.6;
        const points = [{x: startX, y: startY}];
        const segLen = 6 + Math.floor(Math.random() * 8);
        for (let p = 1; p < segLen; p++) {
          const t = p / (segLen - 1);
          // straight interpolation with jitter
          const ix = startX + (endX - startX) * t + (Math.random() - 0.5) * w * 0.04;
          const iy = startY + (endY - startY) * t + (Math.random() - 0.5) * h * 0.04;
          points.push({x: ix, y: iy});
        }
        lightningSegments.push({
          points,
          width: 2.2 + Math.random() * 3.6,
          hue: (document.documentElement.getAttribute('data-theme') === 'dark') ? 200 + Math.random() * 20 : 40 + Math.random() * 10,
          alpha: 0.85 + Math.random() * 0.25,
        });
      }

      // set flash and life
      lightningActive = true;
      lightningLife = 8 + Math.floor(Math.random() * 10);
      flashAlpha = 0.95;

      // spawn a quick burst of bright sparks from the main lightning location
      // pick mid-point of first segment to seed sparks
      const seg = lightningSegments[0];
      const mid = seg.points[Math.floor(seg.points.length / 2)];
      for (let i = 0; i < 28; i++) {
        const d = 1 - Math.random(); // foreground bias
        const depth = 0.1 + d * 0.2;
        embers.push({
          x: mid.x + (Math.random() - 0.5) * 40,
          y: mid.y + (Math.random() - 0.5) * 40,
          vx: (Math.random() - 0.5) * (2 + Math.random() * 4),
          vy: (Math.random() - 0.5) * (2 + Math.random() * 4),
          size: 0.6 + Math.random() * 2.8,
          alpha: 0.5 + Math.random() * 0.6,
          hue: seg.hue,
          depth,
        });
      }
    };

    let stopped = false;
    let last = performance.now();

    const lerp = (a:number, b:number, t:number) => a + (b - a) * t;

    const frame = (now: number) => {
      if (stopped) return;
      const dt = Math.min(40, now - last);
      last = now;

      // lightning scheduling
      if (!lightningActive && now >= nextLightningAt) {
        // small chance to actually strike (prevents patterns), else reschedule
        if (Math.random() > 0.2) {
          triggerLightning();
        }
        nextLightningAt = now + 3000 + Math.random() * 22000; // schedule next
      }

      // smooth inputs
      smoothedMouseX = lerp(smoothedMouseX, targetMouseX, 0.08);
      smoothedMouseY = lerp(smoothedMouseY, targetMouseY, 0.08);
      smoothedScrollY = lerp(smoothedScrollY, targetScrollY, 0.06);

      const relX = (smoothedMouseX - w / 2) / (w / 2);
      const relY = (smoothedMouseY - h / 2) / (h / 2);
      const maxGlobalShiftX = Math.min(40, w * 0.03);
      const maxGlobalShiftY = Math.min(24, h * 0.02);
      const globalShiftX = relX * maxGlobalShiftX;
      const globalShiftY = relY * maxGlobalShiftY + (smoothedScrollY - (window.innerHeight || 0)) * 0.02;

      canvas.style.transform = `translate3d(${globalShiftX}px, ${globalShiftY}px, 0)`;

      ctx.clearRect(0, 0, w, h);

      // Draw embers
      for (let i = embers.length - 1; i >= 0; i--) {
        const e = embers[i];
        const parallaxFactor = (1 - e.depth) * 0.6;
        const drawX = e.x + globalShiftX * parallaxFactor;
        const drawY = e.y + globalShiftY * parallaxFactor;

        e.x += e.vx * (dt / 16);
        e.y += e.vy * (dt / 16);
        e.vy -= 0.0009 * (dt / 16);
        e.alpha -= 0.0009 * (dt / 16); // decay a little faster for sparks

        const r = Math.max(1.6, e.size * 5);
        const grad = ctx.createRadialGradient(drawX, drawY, 0, drawX, drawY, r);
        grad.addColorStop(0, `hsla(${e.hue},100%,68%,${Math.max(0, e.alpha)})`);
        grad.addColorStop(0.25, `hsla(${e.hue - 18},95%,52%,${Math.max(0, e.alpha * 0.45)})`);
        grad.addColorStop(1, 'rgba(0,0,0,0)');
        ctx.fillStyle = grad;
        ctx.beginPath();
        ctx.arc(drawX, drawY, r, 0, Math.PI * 2);
        ctx.fill();

        if (e.alpha <= 0 || e.y + e.size < -120 || e.x < -160 || e.x > w + 160) embers.splice(i, 1);
      }

      // Draw lightning if active
      if (lightningActive && lightningSegments.length) {
        ctx.save();
        ctx.globalCompositeOperation = 'lighter';
        for (let seg of lightningSegments) {
          // draw multiple passes for inner glow + core
          for (let pass = 0; pass < 3; pass++) {
            ctx.beginPath();
            const pts = seg.points;
            ctx.moveTo(pts[0].x, pts[0].y);
            for (let k = 1; k < pts.length; k++) {
              ctx.lineTo(pts[k].x, pts[k].y);
            }
            const wFactor = seg.width * (1.2 - pass * 0.5);
            ctx.lineWidth = Math.max(1, wFactor + Math.random() * 1.2);
            // gradient along first->last
            const first = pts[0], lastPt = pts[pts.length - 1];
            const g = ctx.createLinearGradient(first.x, first.y, lastPt.x, lastPt.y);
            const hue = seg.hue;
            if (document.documentElement.getAttribute('data-theme') === 'dark') {
              g.addColorStop(0, `rgba(${hue},${hue},255,${seg.alpha * 0.9})`);
              g.addColorStop(0.5, `rgba(255,255,255,${seg.alpha})`);
              g.addColorStop(1, `rgba(180,220,255,${seg.alpha * 0.6})`);
            } else {
              g.addColorStop(0, `rgba(255,220,180,${seg.alpha * 0.9})`);
              g.addColorStop(0.5, `rgba(255,255,255,${seg.alpha})`);
              g.addColorStop(1, `rgba(255,200,150,${seg.alpha * 0.6})`);
            }
            ctx.strokeStyle = g;
            ctx.shadowColor = 'rgba(255,255,255,0.8)';
            ctx.shadowBlur = 18 + Math.random() * 22;
            ctx.stroke();
          }
        }
        ctx.restore();
      }

      // Full-screen flash overlay (fade quickly)
      if (flashAlpha > 0.001) {
        ctx.save();
        ctx.globalCompositeOperation = 'screen';
        ctx.fillStyle = `rgba(255,255,255,${Math.min(1, flashAlpha)})`;
        ctx.fillRect(0, 0, w, h);
        ctx.restore();
        // decay flash
        flashAlpha *= 0.7;
      }

      // lightning life decrement / end
      if (lightningActive) {
        lightningLife--;
        if (lightningLife <= 0 && flashAlpha < 0.02) {
          lightningActive = false;
          lightningSegments = [];
        }
      }

      // spawn more embers
      if (Math.random() < 0.95) spawn();

      requestAnimationFrame(frame);
    };

    const raf = requestAnimationFrame(frame);

    const onResize = () => {
      w = Math.max(300, canvas.clientWidth || window.innerWidth);
      h = Math.max(200, canvas.clientHeight || window.innerHeight);
      canvas.width = Math.floor(w * DPR);
      canvas.height = Math.floor(h * DPR);
      ctx.setTransform(DPR, 0, 0, DPR, 0, 0);
    };
    const onMouse = (ev: MouseEvent) => {
      targetMouseX = ev.clientX;
      targetMouseY = ev.clientY;
    };
    const onScroll = () => {
      targetScrollY = window.scrollY || 0;
    };

    window.addEventListener('resize', onResize, {passive: true});
    window.addEventListener('mousemove', onMouse, {passive: true});
    window.addEventListener('scroll', onScroll, {passive: true});

    return () => {
      stopped = true;
      window.removeEventListener('resize', onResize);
      window.removeEventListener('mousemove', onMouse);
      window.removeEventListener('scroll', onScroll);
      cancelAnimationFrame(raf);
    };
  }, [enabled]);

  return ref;
}

/* ---------- Topbar ---------- */
function Topbar(): JSX.Element {
  return (
    <header className="hellish-topbar" role="banner" aria-label="Hellish navigation">
      <div className="hellish-brand">
        <svg className="hellish-logo" viewBox="0 0 24 24" aria-hidden>
          <defs>
            <linearGradient id="lg" x1="0" x2="1">
              <stop offset="0" stopColor="#ff7b50" />
              <stop offset="1" stopColor="#ff2d2d" />
            </linearGradient>
          </defs>
          <rect x="2" y="4" width="20" height="16" rx="4" fill="url(#lg)" />
        </svg>
        <strong className="brand-text">Hellish</strong>
      </div>

      <nav className="hellish-links" aria-label="primary navigation">
        <Link to="/docs/intro" className="nav-link">Docs</Link>
        <Link to="/docs/getting-started" className="nav-link">Quick Start</Link>
        <a href="#showcase" className="nav-link">Showcase</a>
        <a href="https://github.com/your-org/hellish" className="nav-link nav-ghost" target="_blank" rel="noreferrer">GitHub</a>
        <ColorModeToggle />
      </nav>
    </header>
  );
}

/* ---------- Main Page Component ---------- */
export default function Home(): JSX.Element {
  const [mounted, setMounted] = useState(false);
  const particleRef = useEmberCanvas(true);
  const cardRefs = useRef<Record<string, HTMLDivElement | null>>({});
  const [revealed, setRevealed] = useState<Record<string, boolean>>({});

  // per-card animation state (not rendered) for smooth dephased motion
  const animState = useRef<Record<string, {
    curX: number;
    curY: number;
    targetX: number;
    targetY: number;
    raf: number | null;
    phase: number;
    last: number | null;
  }>>({});

  // ensure we cancel RAFs on unmount
  useEffect(() => {
    return () => {
      Object.values(animState.current).forEach((s) => {
        if (s.raf) cancelAnimationFrame(s.raf);
      });
    };
  }, []);

  const startCardLoop = (id: string, el: HTMLElement) => {
    const s = animState.current[id];
    if (!s) return;
    if (s.raf) return;

    s.last = performance.now();
    const loop = (now: number) => {
      const dt = Math.min(40, now - (s.last ?? now));
      s.last = now;

      // slow smoothing (low lerp) to keep motion calm
      const lerp = 0.08; // smaller => slower follow
      s.curX += (s.targetX - s.curX) * lerp;
      s.curY += (s.targetY - s.curY) * lerp;

      // slow, gentle oscillation using per-card phase so cards are dephased
      const t = now / 1000;
      const oscX = Math.sin(t * 0.9 + s.phase) * 0.6; // gentle wobble
      const oscY = Math.cos(t * 0.7 + s.phase) * 0.45;

      const rotX = s.curX + oscY;
      const rotY = s.curY + oscX;

      el.style.transform = `perspective(900px) rotateX(${rotX}deg) rotateY(${rotY}deg) translateY(-3px)`;

      // stop loop when fully relaxed to neutral (tiny thresholds)
      const nearlyIdle = Math.abs(s.curX) < 0.02 && Math.abs(s.curY) < 0.02 && Math.abs(s.targetX) < 0.02 && Math.abs(s.targetY) < 0.02;
      if (nearlyIdle) {
        // clear transform to restore CSS flow
        el.style.transform = '';
        if (s.raf) {
          cancelAnimationFrame(s.raf);
          s.raf = null;
        }
        return;
      }

      s.raf = requestAnimationFrame(loop);
    };

    s.raf = requestAnimationFrame(loop);
  };

  const computeRimColor = (isDark: boolean, intensity: number) => {
    const baseRgb = isDark ? [0, 191, 255] : [255, 123, 80];
    const maxAlpha = 0.26;
    const alpha = +(maxAlpha * intensity).toFixed(3);
    return `rgba(${baseRgb[0]}, ${baseRgb[1]}, ${baseRgb[2]}, ${alpha})`;
  };

  const runExample = useCallback((id?: string) => {
    // placeholder
    // eslint-disable-next-line no-alert
    alert(`Running example: ${id ?? 'example'}\n\n${SAMPLE_SNIPPET}`);
  }, []);

  const copySnippet = useCallback(async (text = SAMPLE_SNIPPET) => {
    try {
      await navigator.clipboard.writeText(text);
      // eslint-disable-next-line no-alert
      alert('Snippet copied to clipboard');
    } catch {
      // eslint-disable-next-line no-alert
      alert('Copy failed — please copy manually.');
    }
  }, []);

  const onCardMove = (id: string) => (e: React.MouseEvent) => {
    const el = cardRefs.current[id];
    if (!el) return;
    const rect = el.getBoundingClientRect();
    const px = (e.clientX - rect.left) / rect.width;
    const py = (e.clientY - rect.top) / rect.height;
    const rotY = (px - 0.5) * 10;
    const rotX = (0.5 - py) * 6;
    el.style.transform = `perspective(900px) rotateX(${rotX}deg) rotateY(${rotY}deg) translateY(-6px)`;
  };
  const onCardLeave = (id: string) => () => {
    const el = cardRefs.current[id];
    if (!el) return;
    el.style.transform = '';
  };

  // --- Spiral transition overlay state & logic ---
  const spiralRef = useRef<HTMLCanvasElement | null>(null);
  const [spiralRunning, setSpiralRunning] = useState(false);

  const playSpiralAndNavigate = useCallback((href: string) => {
    if (spiralRunning) return;
    const canvas = spiralRef.current;
    if (!canvas) {
      window.location.href = href;
      return;
    }

    setSpiralRunning(true);
    const ctx = canvas.getContext('2d');
    if (!ctx) {
      window.location.href = href;
      return;
    }

    const DPR = Math.max(1, window.devicePixelRatio || 1);
    const w = Math.max(300, window.innerWidth);
    const h = Math.max(200, window.innerHeight);
    canvas.width = Math.floor(w * DPR);
    canvas.height = Math.floor(h * DPR);
    canvas.style.width = `${w}px`;
    canvas.style.height = `${h}px`;
    ctx.setTransform(DPR, 0, 0, DPR, 0, 0);

    let t = 0; // 0..1 progression
    let rafId: number | null = null;
    const duration = 820; // ms for main spiral
    const start = performance.now();

    const centerX = w / 2;
    const centerY = h / 2;
    const maxRadius = Math.sqrt(centerX * centerX + centerY * centerY) * 1.05;

    // theme-aware color (use CSS var fallback)
    const flame1 = getComputedStyle(document.documentElement).getPropertyValue('--flame-color-1') || '#ff7b50';
    const flame2 = getComputedStyle(document.documentElement).getPropertyValue('--flame-color-2') || '#ff2d2d';

    const drawFrame = (now: number) => {
      const elapsed = now - start;
      t = Math.min(1, elapsed / duration);

      // clear with a fast fade so spiral trails appear
      ctx.clearRect(0, 0, w, h);
      ctx.globalCompositeOperation = 'lighter';

      // subtle dimming layer for a cinematic effect
      ctx.fillStyle = `rgba(3,6,12,${0.02 + 0.6 * t})`;
      ctx.fillRect(0, 0, w, h);

      // spiral parameters - spin / wind / growth
      const spins = 2.6 + 3.2 * t; // more winds as it progresses
      const maxTheta = Math.PI * 2 * spins;
      const steps = 360; // drawing resolution
      ctx.lineCap = 'round';

      // stroke gradient
      const g = ctx.createLinearGradient(centerX - maxRadius, centerY - maxRadius, centerX + maxRadius, centerY + maxRadius);
      g.addColorStop(0, flame1.trim());
      g.addColorStop(1, flame2.trim());
      ctx.strokeStyle = g;

      // width easing
      ctx.lineWidth = 1 + (16 * t * (1 - Math.pow(1 - t, 2)));

      ctx.beginPath();
      for (let i = 0; i <= steps; i++) {
        const u = i / steps;
        const theta = u * maxTheta;
        // eased radius: start from small and grow outward
        const radius = (theta / maxTheta) * maxRadius * (0.08 + 0.92 * t);
        // add small oscillation for organic feel
        const wob = Math.sin(theta * 3 + t * 6) * (6 * (1 - t));
        const x = centerX + (radius + wob) * Math.cos(theta + t * 4.0);
        const y = centerY + (radius + wob) * Math.sin(theta + t * 4.0);
        if (i === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
      }
      ctx.stroke();

      // bright core pulse near the end
      if (t > 0.6) {
        const pulse = (Math.sin((elapsed - duration * 0.6) / 80) + 1) * 0.5;
        ctx.beginPath();
        ctx.arc(centerX, centerY, 24 + 120 * (t - 0.6) * pulse, 0, Math.PI * 2);
        ctx.fillStyle = `rgba(255,255,255,${0.08 * pulse})`;
        ctx.fill();
      }

      if (t < 1) {
        rafId = requestAnimationFrame(drawFrame);
      } else {
        // small extra frames for fade-out
        const fadeStart = performance.now();
        const fadeLoop = (now2: number) => {
          const fe = Math.min(1, (now2 - fadeStart) / 360);
          ctx.fillStyle = `rgba(3,6,12,${0.8 * fe})`;
          ctx.fillRect(0, 0, w, h);
          if (fe < 1) rafId = requestAnimationFrame(fadeLoop);
          else {
            // navigate after finish
            window.location.href = href;
          }
        };
        rafId = requestAnimationFrame(fadeLoop);
      }
    };

    rafId = requestAnimationFrame(drawFrame);

    // cleanup if user navigates away or component unmounts
    const cleanup = () => {
      if (rafId) cancelAnimationFrame(rafId);
      setSpiralRunning(false);
    };
    // best effort: if navigation does not happen within 3s, restore state
    const timeoutId = window.setTimeout(() => {
      cleanup();
    }, 3000);

    // clear timeout on unload
    window.addEventListener('pagehide', cleanup, {once: true});
    // note: navigation will typically occur via window.location.href above

  }, [spiralRunning]);

  return (
    <Layout title="Hellish — sh42" description="Hellish — sh42 docs and quick start">
      {/* page-hellish is now a wrapper so footer can sit after main and be pushed to bottom */}
      <div className="page-hellish">
        {/* Spiral overlay canvas (top-level so it covers everything) */}
        <div className={clsx('spiral-overlay', {active: spiralRunning})} aria-hidden={!spiralRunning}>
          <canvas ref={spiralRef} className="spiral-canvas" />
        </div>

        <main className="hero hero--custom">
          <canvas ref={particleRef} className="ember-canvas" aria-hidden />

          <div className="overlay-grad" aria-hidden />

          <Topbar />

          <div className="container hero__content">
            <header className={clsx('hero__header', {mounted})} data-reveal-id="hero-header">
              <GlitchTitle />

              <p className={clsx('hero__subtitle', {typing: mounted})} data-reveal-id="hero-sub">
                <span className="typing-text">Terminal, but alive — fast, minimal, beautiful.</span>
              </p>

              <div className="hero__cta" data-reveal-id="hero-cta">
                <Link
                  className="btn btn--primary btn--glow"
                  to="/docs/getting-started"
                  onClick={(e: React.MouseEvent) => {
                    e.preventDefault();
                    playSpiralAndNavigate('/docs/getting-started');
                  }}
                >
                  Quick start
                </Link>
                <Link className="btn btn--secondary btn--outline" to="/docs/intro">Full docs</Link>
                <a className="btn btn--tertiary" href="#showcase">Showcase</a>
              </div>
            </header>

            <section id="showcase" className="showcase section" data-reveal-id="showcase">
              <div className="showcase__grid" role="list">
                {CARDS.map((card, i) => {
                  const delay = `${i * 140}ms`;
                  return (
                    <div
                      key={card.id}
                      ref={(el) => { cardRefs.current[card.id] = el; }}
                      data-reveal-id={`card-${card.id}`}
                      className={clsx('showcase__card interactive-card', {
                        'reveal-in': revealed[`card-${card.id}`],
                      })}
                      style={{transitionDelay: delay}}
                      onMouseMove={onCardMove(card.id, i)}
                      onMouseLeave={onCardLeave(card.id)}
                      role="listitem"
                      aria-label={card.title}
                    >
                      <h3>{card.title}</h3>
                      <p>{card.description}</p>

                      <div className="btn-row">
                        {card.actions.map((a) =>
                          a.kind === 'run' ? (
                            <button key={a.label} className="btn sample" onClick={() => runExample(card.id)}>
                              {a.label}
                            </button>
                          ) : a.kind === 'copy' ? (
                            <button key={a.label} className="btn sample outline" onClick={() => copySnippet()}>
                              {a.label}
                            </button>
                          ) : (
                            <Link key={a.label} className="btn sample alt" to={a.href ?? '#'}>
                              {a.label}
                            </Link>
                          )
                        )}
                      </div>
                    </div>
                  );
                })}
              </div>
            </section>
          </div>
        </main>

        {/* Footer placed after main so wrapper flex layout can push it to page bottom */}
        <Footer />
      </div>
    </Layout>
  );
}
