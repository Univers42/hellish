import React, { useEffect, useRef, useState } from 'react';
import './glitch-title.css';

type Props = {
  size?: number;
  enabled?: boolean; // <--- new optional toggle prop
};

const GHOST_BUBBLES = [
  "Come chase me!",
  "AAAAH! You hovered me!",
  "You tried again!",
  "Alright, you can touch me now.",
  "👻 Boo! Gotcha.",
];

export default function PossessedS({ size = 1, enabled }: Props): JSX.Element | null {
  // helper: determine whether ghost is enabled
  const isEnabled = React.useMemo(() => {
    // prop takes precedence
    if (typeof enabled === 'boolean') return enabled;
    // runtime global toggle (useful during dev)
    try {
      // eslint-disable-next-line @typescript-eslint/no-explicit-any
      const g = (typeof window !== 'undefined' ? (window as any).__HELLISH_GHOST_ENABLED : undefined);
      if (typeof g !== 'undefined') return Boolean(g);
    } catch {}
    // environment variables (build-time)
    const envVal = typeof process !== 'undefined'
      ? (process.env.REACT_APP_ENABLE_GHOST ?? process.env.NEXT_PUBLIC_ENABLE_GHOST ?? process.env.ENABLE_GHOST)
      : undefined;
    if (typeof envVal !== 'undefined') {
      return !(envVal === 'false' || envVal === '0');
    }
    // DEFAULT: disabled to avoid annoying behavior during development
    return false;
  }, [enabled]);

  // If disabled, render nothing and avoid any side effects
  if (!isEnabled) return null;

  const rootRef = useRef<HTMLDivElement | null>(null);
  const rafRef = useRef<number | null>(null);

  const [pos, setPos] = useState<{ x: number; y: number } | null>(null);
  const velRef = useRef<{ vx: number; vy: number }>({ vx: 0, vy: 0 });
  const targetRef = useRef<{ x: number; y: number } | null>(null);

  const [bubble, setBubble] = useState<string | null>(GHOST_BUBBLES[0]);
  const [state, setState] = useState<'idle' | 'dart' | 'touchable' | 'touched' | 'hidden'>('idle');
  const [interacts, setInteracts] = useState(0);
  const [touchCount, setTouchCount] = useState(0);

  // init: random start position & pick initial target
  useEffect(() => {
    const w = window.innerWidth;
    const h = window.innerHeight;
    const startX = 40 + Math.random() * Math.max(200, w - 80);
    const startY = 40 + Math.random() * Math.max(200, h - 80);
    setPos({ x: startX, y: startY });
    targetRef.current = pickRandomTarget();
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  // pick a random point inside viewport margins
  function pickRandomTarget() {
    const margin = 40;
    const w = window.innerWidth;
    const h = window.innerHeight;
    return {
      x: margin + Math.random() * (Math.max(120, w - margin * 2)),
      y: margin + Math.random() * (Math.max(120, h - margin * 2)),
    };
  }

  // periodically update target (so it wanders naturally)
  useEffect(() => {
    const tick = () => {
      if (!targetRef.current) targetRef.current = pickRandomTarget();
      // occasionally nudge the target to avoid monotony
      if (Math.random() < 0.12) targetRef.current = pickRandomTarget();
    };
    const id = window.setInterval(tick, 1400 + Math.random() * 1600);
    return () => clearInterval(id);
  }, []);

  // main smooth steering loop
  useEffect(() => {
    if (!pos) return;
    let running = true;
    let last = performance.now();
    let x = pos.x;
    let y = pos.y;

    function step(now: number) {
      if (!running) return;
      const dt = Math.min(40, now - last);
      last = now;

      const target = targetRef.current ?? pickRandomTarget();
      // vector to target
      let dx = target.x - x;
      let dy = target.y - y;
      const dist = Math.sqrt(dx * dx + dy * dy) || 0.0001;

      // if close to target, pick a new one (natural wandering)
      if (dist < 28 + Math.random() * 40) {
        targetRef.current = pickRandomTarget();
      }

      // desired velocity toward target (scaled)
      const desiredSpeed = state === 'dart' ? 7.8 : 2.4;
      const desiredVx = (dx / dist) * desiredSpeed;
      const desiredVy = (dy / dist) * desiredSpeed;

      // smooth steering: accelerate toward desired velocity
      const steerStrength = state === 'dart' ? 0.72 : 0.16;
      velRef.current.vx += (desiredVx - velRef.current.vx) * (steerStrength * (dt / 16));
      velRef.current.vy += (desiredVy - velRef.current.vy) * (steerStrength * (dt / 16));

      // limit speed
      const s = Math.sqrt(velRef.current.vx * velRef.current.vx + velRef.current.vy * velRef.current.vy) || 0.0001;
      const maxS = state === 'dart' ? 11 : 5;
      if (s > maxS) {
        velRef.current.vx = (velRef.current.vx / s) * maxS;
        velRef.current.vy = (velRef.current.vy / s) * maxS;
      }

      // integrate
      x += velRef.current.vx * (dt / 16);
      y += velRef.current.vy * (dt / 16);

      // keep inside viewport
      const margin = 22;
      const w = window.innerWidth;
      const h = window.innerHeight;
      if (x < margin) { x = margin; velRef.current.vx = Math.abs(velRef.current.vx) * (0.6 + Math.random() * 0.5); }
      if (x > w - margin) { x = w - margin; velRef.current.vx = -Math.abs(velRef.current.vx) * (0.6 + Math.random() * 0.5); }
      if (y < margin) { y = margin; velRef.current.vy = Math.abs(velRef.current.vy) * (0.6 + Math.random() * 0.5); }
      if (y > h - margin) { y = h - margin; velRef.current.vy = -Math.abs(velRef.current.vy) * (0.6 + Math.random() * 0.5); }

      // gently decay dart state once speed reduces
      if (state === 'dart') {
        const speedNow = Math.sqrt(velRef.current.vx * velRef.current.vx + velRef.current.vy * velRef.current.vy);
        if (speedNow < 3.2) {
          // return to idle after dart finishes
          setState((s) => (s === 'dart' ? 'idle' : s));
          setBubble(GHOST_BUBBLES[0]);
        }
      }

      setPos({ x, y });
      rafRef.current = requestAnimationFrame(step);
    }

    rafRef.current = requestAnimationFrame(step);
    return () => {
      running = false;
      if (rafRef.current) cancelAnimationFrame(rafRef.current);
      rafRef.current = null;
    };
  // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [pos, state]);

  // hover => dart away from pointer (more natural fleeing direction)
  const handleHover = (e: React.MouseEvent) => {
    if (state === 'touched') return;
    // compute vector from pointer to ghost and inverse for flee
    if (!pos) return;
    const px = e.clientX;
    const py = e.clientY;
    let dx = pos.x - px;
    let dy = pos.y - py;
    const d = Math.sqrt(dx * dx + dy * dy) || 0.0001;
    dx /= d; dy /= d;
    // set a target velocity away and set dart state
    velRef.current.vx = dx * (6.8 + Math.random() * 4.8);
    velRef.current.vy = dy * (5.8 + Math.random() * 4.2);
    setState('dart');
    setBubble(GHOST_BUBBLES[1]);
    // increment interactions after a short delay
    setTimeout(() => {
      const next = interacts + 1;
      setInteracts(next);
      if (next >= 3) {
        setState('touchable');
        setBubble(GHOST_BUBBLES[3]);
      } else {
        // If still darting, let loop reduce it; we leave bubble as AAH for a moment
      }
    }, 520 + Math.random() * 520);
  };

  // click when touchable => final reaction
  const handleClick = (e: React.MouseEvent) => {
    e.stopPropagation();
    if (state === 'touchable') {
      setState('touched');
      setBubble(GHOST_BUBBLES[4]);
      // small calm wobble
      velRef.current.vx = (Math.random() - 0.5) * 0.6;
      velRef.current.vy = (Math.random() - 0.5) * 0.6;
      setTimeout(() => setBubble(null), 1800);
    } else {
      // provoke a flee if clicked too early
      handleHover(e);
    }
  };

  if (!pos) return null;

  // color based on state: calm green, scared red, touched yellowish
  const color = state === 'dart' ? '#ff6b6b' : state === 'touched' ? '#ffd27f' : '#9be7a5';
  const rootStyle: React.CSSProperties = {
    position: 'fixed',
    left: pos.x,
    top: pos.y,
    zIndex: 9999,
    width: 40,
    height: 40,
    pointerEvents: 'auto',
    transform: 'translate(-50%,-50%)',
    touchAction: 'manipulation',
  };

  const svgStyle: React.CSSProperties = {
    width: '40px',
    height: '40px',
    display: 'block',
    cursor: state === 'touchable' ? 'pointer' : 'grab',
    color,
    filter: state === 'touched' ? 'drop-shadow(0 6px 12px rgba(0,0,0,0.28))' : 'drop-shadow(0 6px 18px rgba(0,0,0,0.32))',
    transition: 'transform 140ms ease, filter 160ms ease, color 220ms ease',
    // use CSS animation classes for bob/shiver via className
  };

  const bubbleStyle: React.CSSProperties = {
    position: 'absolute',
    left: '50%',
    top: -34,
    transform: 'translateX(-50%)',
    background: 'rgba(32,32,36,0.98)',
    color: '#fff',
    fontSize: '0.72rem',
    padding: '6px 8px',
    borderRadius: 8,
    border: '1px solid rgba(255,255,255,0.04)',
    maxWidth: 140,
    whiteSpace: 'nowrap',
    overflow: 'hidden',
    textOverflow: 'ellipsis',
    boxShadow: '0 6px 22px rgba(0,0,0,0.28)',
    pointerEvents: 'none',
  };

  // add classes for CSS animations: calm bob by default, shiver when darting
  const svgClass = state === 'dart' ? 'ghost-svg ghost-shiver' : 'ghost-svg ghost-bob';

  return (
    <div
      ref={rootRef}
      style={rootStyle}
      onMouseEnter={handleHover}
      onClick={handleClick}
      role="button"
      aria-label={state === 'touchable' ? 'Touch the ghost' : 'Chase the ghost'}
      tabIndex={0}
      onKeyDown={(e) => { if (e.key === 'Enter' || e.key === ' ') handleClick(e as any); }}
    >
      <svg className={svgClass} style={svgStyle} viewBox="0 0 24 24" aria-hidden>
        <g transform="translate(0,0)">
          <path d="M12 2c2.8 0 5 2 5 4.6v6.8c0 1.8-1.6 3.6-3.6 3.6-1 0-1.8-0.6-2.4-1.6-0.6 1-1.4 1.6-2.4 1.6-2 0-3.6-1.8-3.6-3.6V6.6C7 4 9.2 2 12 2z" fill="currentColor" opacity="0.98"/>
          <circle cx="9.6" cy="9.4" r="0.9" fill="#07102a" />
          <circle cx="14.4" cy="9.4" r="0.9" fill="#07102a" />
        </g>
      </svg>

      {bubble && <div style={bubbleStyle} aria-live="polite">{bubble}</div>}
    </div>
  );
}
