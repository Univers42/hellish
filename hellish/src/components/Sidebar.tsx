import React, { useMemo, useState, useCallback, useEffect } from 'react';
import { useLocation, Link } from '@docusaurus/router';
import './sidebar.css';
import * as Lucide from 'lucide-react';
import { motion, AnimatePresence } from 'framer-motion';

type DocLink = {
  type: 'link' | 'doc';
  label: string;
  href?: string;
  id?: string;
  customProps?: Record<string, any>;
};

type Category = {
  type: 'category';
  label: string;
  items: SidebarItem[];
  collapsible?: boolean;
};

type SidebarItem = DocLink | Category;

type Props = {
  items: SidebarItem[];
  title?: string;
  logo?: { src?: string; alt?: string };
  compact?: boolean;
};

function renderIconByName(name?: string, fallback?: React.ReactNode) {
  if (!name) return fallback ?? null;
  // lucide-react exports are PascalCase (e.g., FileText, ChevronDown)
  try {
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    const IconComp = (Lucide as any)[name];
    if (IconComp) return <IconComp className="sid-icon-svg" aria-hidden />;
  } catch {
    // ignore
  }
  return fallback ?? null;
}

function DefaultDocIcon() {
  return renderIconByName('FileText', <svg width="14" height="14" viewBox="0 0 24 24" className="sid-icon-svg"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z" fill="currentColor"/></svg>);
}

function CaretIcon({ open }: { open: boolean }) {
  return renderIconByName(open ? 'ChevronDown' : 'ChevronRight', null);
}

export default function Sidebar({ items, title = 'Hellish', logo, compact = false }: Props) {
  const location = useLocation();
  const pathname = location?.pathname ?? '/';

  const [openMap, setOpenMap] = useState<Record<string, boolean>>({});

  const mkKey = useCallback((parents: string[], label: string) => [...parents, label].join(' / '), []);

  useEffect(() => {
    const map: Record<string, boolean> = {};
    const walk = (nodes: SidebarItem[], parents: string[]) => {
      nodes.forEach((n) => {
        if ((n as Category).type === 'category') {
          const cat = n as Category;
          const key = mkKey(parents, cat.label);
          const contains = (items: SidebarItem[]): boolean => {
            return items.some((c) => {
              if ((c as Category).type === 'category') {
                return contains((c as Category).items);
              }
              const link = c as DocLink;
              if (!link.href) return false;
              return pathname === link.href || pathname.startsWith(link.href);
            });
          };
          if (contains(cat.items)) map[key] = true;
          walk(cat.items, [...parents, cat.label]);
        }
      });
    };
    walk(items, []);
    setOpenMap((s) => ({ ...s, ...map }));
  // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [pathname, items]);

  const toggle = (key: string) => setOpenMap((m) => ({ ...m, [key]: !m[key] }));

  const isActiveLink = (href?: string) => {
    if (!href) return false;
    if (href === pathname) return true;
    return pathname.startsWith(href);
  };

  const handleKeyToggle = (e: React.KeyboardEvent, key: string) => {
    if (e.key === 'Enter' || e.key === ' ') {
      e.preventDefault();
      toggle(key);
    }
  };

  const renderItems = (nodes: SidebarItem[], parents: string[] = [], depth = 0) => {
    return nodes.map((node) => {
      if ((node as Category).type === 'category') {
        const cat = node as Category;
        const key = mkKey(parents, cat.label);
        const open = !!openMap[key];
        // try to read an icon key from category.customProps.icon (optional)
        const iconName = (cat as any).customProps?.icon as string | undefined;
        return (
          <li key={key} className={`sid-cat ${open ? 'open' : ''}`} style={{ ['--depth' as any]: depth }}>
            <button
              type="button"
              className="sid-cat-btn"
              aria-expanded={open}
              aria-controls={`${encodeURIComponent(key)}-panel`}
              onClick={() => toggle(key)}
              onKeyDown={(e) => handleKeyToggle(e, key)}
            >
              <span className="sid-cat-left">
                <span className="sid-cat-icon" aria-hidden>{renderIconByName(iconName, <span className="sid-default-cat-icon">{CaretIcon({ open })}</span>)}</span>
                <span className="sid-cat-label">{cat.label}</span>
              </span>
              <span className="sid-cat-right" aria-hidden>{renderIconByName('ChevronDown')}</span>
            </button>

            <AnimatePresence initial={false}>
              {open && (
                <motion.div
                  className="sid-cat-panel"
                  id={`${encodeURIComponent(key)}-panel`}
                  key="panel"
                  initial={{ height: 0, opacity: 0 }}
                  animate={{ height: 'auto', opacity: 1 }}
                  exit={{ height: 0, opacity: 0 }}
                  transition={{ duration: 0.28, ease: [0.2, 0.9, 0.3, 1] }}
                  style={{ overflow: 'hidden' }}
                >
                  <ul className="sid-list">
                    {renderItems(cat.items, [...parents, cat.label], depth + 1)}
                  </ul>
                </motion.div>
              )}
            </AnimatePresence>
          </li>
        );
      }

      const link = node as DocLink;
      const href = link.href ?? (link.id ? `/docs/${link.id}` : '#');
      const active = isActiveLink(href);
      // icon name may come from customProps.icon or from link.customProps?.icon
      const iconName = (link as any).customProps?.icon as string | undefined;
      return (
        <li key={href + link.label} className={`sid-item ${active ? 'active' : ''}`} style={{ ['--depth' as any]: depth }}>
          <Link className="sid-link" to={href}>
            <span className="sid-link-left">
              <span className="sid-link-icon" aria-hidden>{renderIconByName(iconName, <DefaultDocIcon />)}</span>
              <span className="sid-link-label">{link.label}</span>
            </span>
            {active && <span className="sid-active-bar" aria-hidden />}
          </Link>
        </li>
      );
    });
  };

  return (
    <aside className={`hellish-sidebar ${compact ? 'compact' : ''}`} aria-label="Documentation sidebar">
      <div className="sid-header">
        {logo?.src ? <img src={logo.src} alt={logo.alt ?? 'logo'} className="sid-logo" /> : <div className="sid-logo-mark">H</div>}
        <div className="sid-title-wrap">
          <div className="sid-title">{title}</div>
          <div className="sid-sub">Docs</div>
        </div>
      </div>

      <nav className="sid-nav" aria-label="Docs navigation">
        <ul className="sid-list" role="tree">
          {renderItems(items)}
        </ul>
      </nav>

      <div className="sid-footer">
        <a className="sid-cta" href="/docs/intro">
          <span className="sid-cta-dot" /> Get started
        </a>
      </div>
    </aside>
  );
}
