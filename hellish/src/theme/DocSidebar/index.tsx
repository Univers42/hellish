import React from 'react';
import OriginalDocSidebar from '@theme-original/DocSidebar';
import Sidebar from '@site/src/components/Sidebar';
import type { Props as OriginalProps } from '@theme-original/DocSidebar';
import useDocusaurusContext from '@docusaurus/useDocusaurusContext';

/**
 * Minimal adapter to replace default DocSidebar with our custom Sidebar.
 * Place this file at src/theme/DocSidebar/index.tsx (theme swizzle) — Docusaurus will pick it.
 *
 * The default DocSidebar provides a "sidebar" prop with shape {items: SidebarItem[] }.
 * We pass those items to our Sidebar component.
 */
export default function DocSidebarAdapter(props: OriginalProps) {
  const { siteConfig } = useDocusaurusContext();
  const sidebar = (props as any).sidebar ?? (props as any).sidebarData ?? undefined;

  // If no sidebar available, fall back to the original component
  if (!sidebar || !sidebar.items) {
    // fallback to the original if something unexpected
    return <OriginalDocSidebar {...props} />;
  }

  return (
    <Sidebar
      items={sidebar.items}
      title={siteConfig?.title ?? 'Docs'}
      logo={{ src: '/img/logo.svg', alt: siteConfig?.title ?? 'logo' }}
    />
  );
}
