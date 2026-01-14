import React from 'react';
import {useColorMode} from '@docusaurus/theme-common';

export default function ColorModeToggle(): JSX.Element {
  const {colorMode, setColorMode} = useColorMode();

  const toggle = () => {
    setColorMode(colorMode === 'dark' ? 'light' : 'dark');
  };

  return (
    <button
      className="color-mode-toggle"
      onClick={toggle}
      aria-label={`Switch to ${colorMode === 'dark' ? 'light' : 'dark'} mode`}
    >
      {colorMode === 'dark' ? '🌙' : '☀️'}
    </button>
  );
}
