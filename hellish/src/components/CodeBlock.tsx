import React, { useState, useCallback } from 'react';
import Highlight, { defaultProps, Language } from 'prism-react-renderer';
import themeLight from 'prism-react-renderer/themes/github';
import themeDark from 'prism-react-renderer/themes/dracula';

type Props = {
  children: string;
  language?: Language | string;
  hideLineNumbers?: boolean;
};

export default function CodeBlock({ children, language = 'tsx', hideLineNumbers = false }: Props) {
  const [copied, setCopied] = useState(false);
  const dark = typeof window !== 'undefined' && document.documentElement.getAttribute('data-theme') === 'dark';
  const theme = dark ? themeDark : themeLight;

  const onCopy = useCallback(async () => {
    try {
      await navigator.clipboard.writeText(children.trim());
      setCopied(true);
      setTimeout(() => setCopied(false), 1400);
    } catch {
      // ignore
    }
  }, [children]);

  return (
    <div className="code-block-wrapper" role="region" aria-label="code block">
      <button className="copy-button" onClick={onCopy} aria-label="Copy code">
        {copied ? 'Copied' : 'Copy'}
      </button>
      <Highlight {...(defaultProps as any)} theme={theme as any} code={children.trim()} language={language as Language}>
        {({ className, style, tokens, getLineProps, getTokenProps }) => (
          <pre className={className} style={{ ...style, padding: 0 }}>
            <code>
              {tokens.map((line, i) => (
                <div key={i} className="code-block-line" {...getLineProps({ line, key: i })}>
                  {!hideLineNumbers && <span className="line-number">{i + 1}</span>}
                  <span className="line-content">
                    {line.map((token, key) => (
                      <span key={key} {...getTokenProps({ token, key })} />
                    ))}
                  </span>
                </div>
              ))}
            </code>
          </pre>
        )}
      </Highlight>
    </div>
  );
}
