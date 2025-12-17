import React, { useState } from 'react';

type Variant = 'info' | 'success' | 'warn' | 'danger';

type Props = {
  variant?: Variant;
  title?: React.ReactNode;
  collapsible?: boolean;
  children?: React.ReactNode;
};

const ICONS: Record<Variant, string> = {
  info: 'ℹ️',
  success: '✅',
  warn: '⚠️',
  danger: '🔥',
};

export default function Callout({ variant = 'info', title, collapsible = false, children }: Props) {
  const [open, setOpen] = useState(true);
  return (
    <div className={`callout ${variant}`} role="region" aria-label={`${variant} callout`}>
      <div className="icon" aria-hidden>{ICONS[variant]}</div>
      <div style={{ flex: 1 }}>
        {title && <h4>{title}</h4>}
        {(!collapsible || open) && <div>{children}</div>}
      </div>
      {collapsible && (
        <button
          className="toggle"
          aria-expanded={open}
          onClick={() => setOpen((v) => !v)}
          title={open ? 'Collapse' : 'Expand'}
        >
          {open ? '▾' : '▸'}
        </button>
      )}
    </div>
  );
}
