import React, { useState, useRef } from 'react';
import './glitch-title.css';
import PossessedS from './PossessedS';

export default function GlitchTitle(): JSX.Element {
  const [showJoke, setShowJoke] = useState(false);
  const containerRef = useRef<HTMLDivElement | null>(null);

  const toggleJoke = () => setShowJoke(!showJoke);

  return (
    <div className="glitch-title-container" ref={containerRef}>
      <svg
        className="glitch-svg"
        viewBox="0 0 400 100"
        xmlns="http://www.w3.org/2000/svg"
        aria-label="Hellish"
        role="img"
      >
        <defs>
          <linearGradient id="mainGrad" x1="0%" y1="0%" x2="100%" y2="0%">
            <stop offset="0%" stopColor="var(--flame-color-1)" />
            <stop offset="100%" stopColor="var(--flame-color-2)" />
          </linearGradient>
          <linearGradient id="ghost1Grad" x1="0%" y1="0%" x2="100%" y2="0%">
            <stop offset="0%" stopColor="var(--flame-color-ghost-1)" />
            <stop offset="100%" stopColor="var(--flame-color-ghost-2)" />
          </linearGradient>
          <linearGradient id="ghost2Grad" x1="0%" y1="0%" x2="100%" y2="0%">
            <stop offset="0%" stopColor="var(--flame-color-ghost-2)" />
            <stop offset="100%" stopColor="var(--flame-color-ghost-1)" />
          </linearGradient>
          {/* Glitch filter with glow */}
          <filter id="glitchFilter" x="-50%" y="-50%" width="200%" height="200%">
            <feTurbulence baseFrequency="0.05 0.15" numOctaves="3" result="noise">
              <animate attributeName="baseFrequency" values="0.05 0.15;0.08 0.12;0.05 0.15" dur="2s" repeatCount="indefinite" />
            </feTurbulence>
            <feDisplacementMap in="SourceGraphic" in2="noise" scale="5" result="displaced" />
            <feGaussianBlur in="displaced" stdDeviation="2" result="blurred" />
            <feColorMatrix in="blurred" type="matrix" values="1 0 0 0 0  0 1 0 0 0  0 0 1 0 0  0 0 0 1.5 0" result="glow" />
            <feMerge>
              <feMergeNode in="glow" />
              <feMergeNode in="displaced" />
            </feMerge>
          </filter>
        </defs>

        {/* Glitch squares for reinforcement */}
        <rect x="50" y="20" width="20" height="20" fill="var(--flame-color-1)" opacity="0.7" className="glitch-square-1" />
        <rect x="150" y="30" width="15" height="15" fill="var(--flame-color-2)" opacity="0.6" className="glitch-square-2" />

        {/* Main text: "Hellish" WITHOUT the 's' - the S is independent */}
        <text
          x="50%"
          y="60%"
          textAnchor="middle"
          fontSize="48"
          fontWeight="800"
          fill="url(#mainGrad)"
          filter="url(#glitchFilter)"
          className="glitch-main-text"
        >
          Hellish
        </text>

        {/* Ghost 1: "sHellish" */}
        <text
          x="50%"
          y="60%"
          textAnchor="middle"
          fontSize="48"
          fontWeight="800"
          fill="url(#ghost1Grad)"
          filter="url(#glitchFilter)"
          className="glitch-ghost-1"
        >
          Hellish
        </text>

        {/* Ghost 2: "sHellish" */}
        <text
          x="50%"
          y="60%"
          textAnchor="middle"
          fontSize="48"
          fontWeight="800"
          fill="url(#ghost2Grad)"
          filter="url(#glitchFilter)"
          className="glitch-ghost-2"
        >
          Hellish
        </text>
      </svg>

      {/* Possessed, animated, clickable S that lives outside the SVG flow */}
      <PossessedS targetRef={containerRef} />

      {showJoke && (
        <div className="joke-bubble" role="tooltip" aria-live="polite">
          <div className="joke-bubble-content">
            <p>
              Hellish is a <strong>shell</strong>, but it's <em>hellish</em> because it's a shell in hell! 🔥🐚😈<br />
              Flip it around: <span className="joke-highlight">shellish</span>, <span className="joke-highlight">shellishell</span> – it's a total shell game!<br />
              Get it? <strong>sHELL</strong>ish! You're doomed to love it! 😂💥
            </p>
          </div>
          <div className="joke-bubble-arrow"></div>
        </div>
      )}
    </div>
  );
}
