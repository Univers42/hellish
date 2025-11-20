import React from 'react';

type Props = {
  src: string; // url to a prebuilt demo (CodeSandbox, iframe bundle)
  height?: number;
  title?: string;
};

export default function Playground({ src, height = 360, title = 'Playground' }: Props) {
  return (
    <div style={{ border: '1px solid var(--glass-border)', borderRadius: 8, overflow: 'hidden' }}>
      <iframe
        title={title}
        src={src}
        style={{ width: '100%', height, border: 0 }}
        sandbox="allow-scripts allow-same-origin allow-forms"
      />
    </div>
  );
}
