import React from 'react';
import type { ReactNode } from 'react';

/**
 * Demo uses react-live to render small React examples inline.
 * Security: react-live executes code in the page — don't render untrusted user content.
 * If you need safer isolation, use Playground (iframe) component below.
 */

type Props = {
  code: string;
  scope?: Record<string, any>;
  noInline?: boolean;
  previewOnly?: boolean;
  caption?: ReactNode;
};

export default function Demo({ code, scope = {}, noInline = false, previewOnly = false, caption }: Props) {
  // lazy require to avoid hard dependency during build-time if not installed
  let LiveProvider: any;
  let LiveEditor: any;
  let LiveError: any;
  let LivePreview: any;
  try {
    // eslint-disable-next-line @typescript-eslint/no-var-requires
    const live = require('react-live');
    LiveProvider = live.LiveProvider;
    LiveEditor = live.LiveEditor;
    LiveError = live.LiveError;
    LivePreview = live.LivePreview;
  } catch (err) {
    LiveProvider = null;
  }

  if (!LiveProvider) {
    return (
      <div style={{ border: '1px dashed var(--glass-border)', padding: 12, borderRadius: 8 }}>
        <p style={{ margin: 0 }}>react-live is not installed. Install <code>react-live</code> to enable live demos.</p>
        <pre style={{ marginTop: 8, whiteSpace: 'pre-wrap' }}>{code}</pre>
      </div>
    );
  }

  return (
    <LiveProvider code={code} scope={scope} noInline={noInline}>
      <div style={{ borderRadius: 10, overflow: 'hidden', border: '1px solid var(--glass-border)', margin: '12px 0' }}>
        <div style={{ padding: 12, background: 'var(--panel)' }}>
          <LivePreview />
        </div>
        {!previewOnly && (
          <div style={{ background: 'var(--code-bg)' }}>
            <LiveEditor style={{ fontFamily: 'monospace', fontSize: 13 }} />
            <LiveError style={{ color: 'var(--danger)', padding: 8 }} />
          </div>
        )}
        {caption && <div style={{ padding: 10, fontSize: 13, color: 'var(--muted)' }}>{caption}</div>}
      </div>
    </LiveProvider>
  );
}
