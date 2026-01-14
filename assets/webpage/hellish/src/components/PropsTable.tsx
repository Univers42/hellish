import React from 'react';

type PropRow = {
  name: string;
  type?: string;
  default?: string;
  description?: React.ReactNode;
  required?: boolean;
};

type Props = {
  rows: PropRow[];
  zebra?: boolean;
};

export default function PropsTable({ rows, zebra = true }: Props) {
  return (
    <div style={{ overflowX: 'auto', borderRadius: 8 }}>
      <table style={{ width: '100%', borderCollapse: 'collapse' }}>
        <thead>
          <tr>
            <th style={{ textAlign: 'left', padding: 8 }}>Prop</th>
            <th style={{ textAlign: 'left', padding: 8 }}>Type</th>
            <th style={{ textAlign: 'left', padding: 8 }}>Default</th>
            <th style={{ textAlign: 'left', padding: 8 }}>Description</th>
          </tr>
        </thead>
        <tbody>
          {rows.map((r, i) => (
            <tr key={r.name} style={{ background: zebra && i % 2 ? 'rgba(255,255,255,0.01)' : undefined }}>
              <td style={{ padding: 8 }}><code>{r.name}{r.required ? ' *' : ''}</code></td>
              <td style={{ padding: 8, color: 'var(--muted)' }}><code>{r.type ?? '-'}</code></td>
              <td style={{ padding: 8, color: 'var(--muted)' }}><code>{r.default ?? '-'}</code></td>
              <td style={{ padding: 8, color: 'var(--muted)' }}>{r.description ?? '-'}</td>
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  );
}
