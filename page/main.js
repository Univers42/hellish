// One-article controller: aside-driven show/hide (no SPA history)
// Updated: deactivate inactive articles (inert + aria-hidden) and stack them visually.
(function () {
  'use strict';

  const qs = (s, root = document) => root.querySelector(s);
  const qsa = (s, root = document) => Array.from(root.querySelectorAll(s));

  const stage = qs('.article-stage');
  if (!stage) return;
  const articles = qsa('article[id]', stage);
  const articleById = new Map(articles.map(a => [a.id, a]));

  // helper to deactivate an article element
  function deactivateArticle(el) {
    el.classList.remove('active');
    el.setAttribute('aria-hidden', 'true');
    // tabindex -1 to prevent focus
    el.setAttribute('tabindex', '-1');
    // set inert if supported (best-effort)
    try { el.inert = true; } catch (e) { /* inert may not be supported; CSS handles pointer-events */ }
  }

  // helper to activate an article element
  function activateArticle(el) {
    el.classList.add('active');
    el.removeAttribute('aria-hidden');
    el.removeAttribute('tabindex');
    try { el.inert = false; } catch (e) { /* ignore */ }
    // focus for keyboard users but avoid scrolling
    try { el.focus({ preventScroll: true }); } catch (e) { /* ignore */ }
  }

  // initialize: mark all articles inert/hidden
  articles.forEach(a => deactivateArticle(a));

  function showArticleById(id) {
    const target = articleById.get(id);
    if (!target) return;
    articles.forEach(a => {
      if (a === target) activateArticle(a);
      else deactivateArticle(a);
    });
    // update aside active classes
    qsa('.doc-nav a[href^="#"]').forEach(link => {
      link.classList.toggle('active', link.getAttribute('href') === `#${id}`);
    });
  }

  { 
    // expose SPA helpers so other UI controls (prev/next) can use them
    try {
      window.hellishShowArticle = showArticleById;
      window.hellishArticleOrder = articles.map(a => a.id);
    } catch (e) { /* noop in restricted envs */ }
  }

  // Bind aside links only — aside controls which article is visible
  qsa('.doc-nav a[href^="#"]').forEach(link => {
    link.addEventListener('click', (e) => {
      const href = link.getAttribute('href') || '';
      if (!href.startsWith('#')) return;
      const id = href.slice(1);
      if (!articleById.has(id)) return;
      e.preventDefault();
      showArticleById(id);

      // close mobile menu if open
      const mm = qs('#mobileMenu');
      if (mm && mm.classList.contains('active')) mm.classList.remove('active');
    });
  });

  // Optional aside search/filter
  const asideSearch = qs('#asideSearch');
  if (asideSearch) {
    asideSearch.addEventListener('input', (e) => {
      const q = String(e.target.value || '').trim().toLowerCase();
      qsa('.doc-nav a').forEach(l => {
        const txt = (l.textContent || '').trim().toLowerCase();
        l.style.display = (!q || txt.includes(q)) ? 'block' : 'none';
      });
    });
  }

  // Theme toggle persistence
  const themeToggle = qs('#themeToggle');
  function setTheme(t) {
    document.documentElement.setAttribute('data-theme', t);
    try { localStorage.setItem('hellish-doc-theme', t); } catch (e) {}
  }
  if (themeToggle) {
    themeToggle.addEventListener('click', () => {
      const next = document.documentElement.getAttribute('data-theme') === 'dark' ? 'light' : 'dark';
      setTheme(next);
    });
  }
  try {
    const saved = localStorage.getItem('hellish-doc-theme');
    if (saved) setTheme(saved);
  } catch (e) {}

  // initial show: hash if present and matches, otherwise first article
  document.addEventListener('DOMContentLoaded', () => {
    const hash = (location.hash || '').replace('#','');
    const initial = articleById.has(hash) ? hash : (articles[0] && articles[0].id);
    if (initial) showArticleById(initial);
  });

})();

// --- Modal helpers for footer links (append after existing controller) ---
(function () {
  const qs = (s, root = document) => root.querySelector(s);
  const qsa = (s, root = document) => Array.from((root || document).querySelectorAll(s));
  const overlay = qs('#modalOverlay');
  const body = qs('#modalBody');
  const titleEl = qs('#modalTitle');
  const closeBtn = qs('#modalClose');
  const closeBtn2 = qs('#modalCloseBtn');
  const templates = qs('#modal-templates');

  if (!overlay || !body || !templates) return;

  function openModal(name) {
    const tpl = qs(`#tpl-${name}`, templates);
    if (!tpl) return;
    // set title from first heading or default
    const h = tpl.querySelector('h3');
    titleEl.textContent = h ? h.textContent : 'Info';
    body.innerHTML = tpl.innerHTML.replace(/<h3>.*?<\/h3>/i, ''); // remove duplicated heading in body
    overlay.classList.add('active');
    overlay.style.visibility = 'visible';
    overlay.style.opacity = '1';
    document.documentElement.setAttribute('data-modal-open', 'true');
    try { document.body.style.overflow = 'hidden'; } catch (e) {}
    // focus modal close for accessibility
    (closeBtn || closeBtn2).focus();
  }

  function closeModal() {
    overlay.classList.remove('active');
    overlay.style.opacity = '0';
    overlay.style.visibility = 'hidden';
    document.documentElement.removeAttribute('data-modal-open');
    try { document.body.style.overflow = ''; } catch (e) {}
  }

  // click handlers for footer modal links
  qsa('a[data-modal]').forEach(a => {
    a.addEventListener('click', (ev) => {
      ev.preventDefault();
      const key = a.getAttribute('data-modal');
      if (!key) return;
      openModal(key);
    });
  });

  // close buttons
  if (closeBtn) closeBtn.addEventListener('click', closeModal);
  if (closeBtn2) closeBtn2.addEventListener('click', closeModal);

  // click outside modal to close
  overlay.addEventListener('click', (e) => {
    if (e.target === overlay) closeModal();
  });

  // ESC to close
  document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape' && overlay.style.visibility === 'visible') closeModal();
  });
})();

// Entry overlay controller: show on first load, manage selection -> reveal SPA content
(function () {
  const qs = (s, r=document) => r.querySelector(s);
  const qsa = (s, r=document) => Array.from((r||document).querySelectorAll(s));
  const overlay = qs('#entryOverlay');
  if (!overlay) return;

  const entryInner = qs('.entry-inner', overlay);
  const closeBtn = qs('#entryClose', overlay);
  const tiles = qsa('.entry-tile', overlay);
  const mainEl = qs('main');
  const brand = qs('.header-brand') || qs('.navbar-brand');

  function openOverlay() {
    overlay.classList.remove('hidden');
    document.documentElement.setAttribute('data-entry', 'open');
    if (mainEl) mainEl.setAttribute('aria-hidden', 'true');
    // trap focus on overlay
    try { (tiles[0] || overlay).focus(); } catch (e) {}
  }

  function closeOverlay() {
    overlay.classList.add('hidden');
    document.documentElement.removeAttribute('data-entry');
    if (mainEl) mainEl.removeAttribute('aria-hidden');
  }

  // Tile click: either navigate to article id or trigger action
  tiles.forEach(t => {
    t.addEventListener('click', (e) => {
      const target = t.getAttribute('data-target');
      const action = t.getAttribute('data-action');
      // hide overlay first
      closeOverlay();
      // if target matches an article id, use existing SPA showArticleById (defined above)
      if (target) {
        // small delay so overlay fade completes
        setTimeout(() => {
          const link = document.querySelector(`.doc-nav a[href="#${target}"]`);
          if (link) link.click();
          else if (typeof showArticleById === 'function') showArticleById(target);
        }, 220);
      } else if (action) {
        // fallback actions: "contribute" -> open contributing section; "search" -> focus aside search
        if (action === 'contribute') {
          setTimeout(() => {
            const link = document.querySelector(`.doc-nav a[href="#contributing"]`);
            if (link) link.click();
          }, 220);
        } else if (action === 'search') {
          setTimeout(() => {
            const s = document.getElementById('asideSearch');
            if (s) { s.focus(); } else { window.scrollTo({top:0,behavior:'smooth'}); }
          }, 220);
        }
      }
    });
  });

  // close overlay (skip)
  if (closeBtn) closeBtn.addEventListener('click', () => {
    closeOverlay();
    // show default article after skip
    setTimeout(() => {
      const first = document.querySelector('.article-stage article[id]');
      if (first && typeof showArticleById === 'function') showArticleById(first.id);
    }, 220);
  });

  // ESC to close overlay
  document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape' && !overlay.classList.contains('hidden')) {
      closeBtn && closeBtn.focus();
      closeOverlay();
    }
  });

  // brand click reopens overlay
  if (brand) {
    brand.addEventListener('click', (e) => {
      e.preventDefault();
      openOverlay();
    });
  }

  // show overlay on first load (unless user explicitly has closed it before)
  document.addEventListener('DOMContentLoaded', () => {
    // if user previously dismissed overlay, check localStorage
    let seen = false;
    try { seen = !!localStorage.getItem('hellish:entry-seen'); } catch (e) {}
    if (!seen) {
      openOverlay();
      try { localStorage.setItem('hellish:entry-seen', '1'); } catch (e) {}
    } else {
      // if already seen, reveal default article (keeps prior behavior)
      const hash = (location.hash || '').replace('#','');
      const firstId = (document.querySelector('.article-stage article[id]') || {}).id;
      const initial = document.querySelector(`.article-stage article#${hash}`) ? hash : firstId;
      if (initial && typeof showArticleById === 'function') showArticleById(initial);
    }
  });
})();

// Header / nav enhancements: wire header links to aside SPA, mobile menu toggle, header shrink on scroll
(function () {
  const qs = (s, root=document) => root.querySelector(s);
  const qsa = (s, root=document) => Array.from((root||document).querySelectorAll(s));

  const mobileToggle = qs('#mobileToggle');
  const mobileMenu = qs('#mobileMenu');
  const header = qs('.site-header') || qs('.navbar');

  // mobile toggle behavior (aria + class)
  if (mobileToggle && mobileMenu) {
    mobileToggle.addEventListener('click', (e) => {
      const open = mobileMenu.classList.toggle('active');
      mobileToggle.classList.toggle('active', open);
      mobileToggle.setAttribute('aria-expanded', String(!!open));
      // lock body scroll a bit when menu open
      try { document.body.style.overflow = open ? 'hidden' : ''; } catch (err) {}
    });
    // close mobile menu when a mobile link is clicked
    qsa('.mobile-menu a, .mobile-menu .mobile-nav-link').forEach(a => {
      a.addEventListener('click', () => {
        mobileMenu.classList.remove('active');
        mobileToggle.classList.remove('active');
        try { document.body.style.overflow = ''; } catch (e) {}
      });
    });
  }

  // header nav: delegate clicks to aside nav so SPA logic is reused
  qsa('.navbar-nav .nav-link').forEach(link => {
    link.addEventListener('click', (e) => {
      const href = link.getAttribute('href') || '';
      if (!href.startsWith('#')) return; // external or non-anchor
      const id = href.slice(1);
      // find aside link that controls SPA
      const asideLink = document.querySelector(`.doc-nav a[href="#${id}"]`);
      if (asideLink) {
        e.preventDefault();
        asideLink.click(); // reuse SPA behavior
        // close mobile menu if open
        if (mobileMenu && mobileMenu.classList.contains('active')) {
          mobileMenu.classList.remove('active');
          mobileToggle && mobileToggle.classList.remove('active');
        }
        // ensure header overlay (entry) hidden if present
        const overlay = document.getElementById('entryOverlay');
        if (overlay && !overlay.classList.contains('hidden')) overlay.classList.add('hidden');
      }
    });
  });

  // active state syncing (when aside links are clicked, mirror to header)
  document.addEventListener('click', (ev) => {
    const target = ev.target;
    if (!target) return;
    // if an aside nav link was clicked, mirror active class
    if (target.matches && target.matches('.doc-nav a[href^="#"]')) {
      const href = target.getAttribute('href');
      qsa('.navbar-nav .nav-link').forEach(h => h.classList.toggle('active', h.getAttribute('href') === href));
    }
  });

  // shrink header on scroll for subtle effect
  let lastScroll = 0;
  window.addEventListener('scroll', () => {
    const y = window.scrollY || window.pageYOffset;
    if (!header) return;
    if (y > 80 && y > lastScroll) header.classList.add('shrink');
    else header.classList.remove('shrink');
    lastScroll = y;
  }, { passive: true });

})();

// --- Bubble controller: handle click/drag/expand for the floating bubble ---
(function () {
  const qs = (s, r=document) => r.querySelector(s);
  const bubble = qs('#docBubble');
  const handle = qs('#bubbleHandle');
  const card = qs('#bubbleCard');

  if (!bubble || !handle || !card) return;

  let expanded = false;
  function setExpanded(val) {
    expanded = !!val;
    bubble.classList.toggle('expanded', expanded);
    handle.setAttribute('aria-expanded', String(!!expanded));
    bubble.setAttribute('aria-hidden', String(!expanded));
    // when expanded on small screens, avoid body scroll behind dialog
    if (expanded && window.innerWidth < 768) try { document.body.style.overflow = 'hidden'; } catch(e){}
    else try { document.body.style.overflow = ''; } catch(e){}

    if (expanded) {
      // move focus into panel (delay to allow transition)
      setTimeout(() => {
        const f = card.querySelector('a, button, [tabindex]:not([tabindex="-1"])');
        if (f) try { f.focus(); } catch(e){}
      }, 220);
    }
  }

  // toggle on click
  handle.addEventListener('click', (e) => {
    e.preventDefault();
    setExpanded(!expanded);
  });

  // close on ESC
  document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape' && expanded) {
      setExpanded(false);
      try { handle.focus(); } catch(e){}
    }
  });

  // click outside to close
  document.addEventListener('click', (e) => {
    if (!expanded) return;
    if (!bubble.contains(e.target) && !card.contains(e.target)) {
      setExpanded(false);
    }
  });

  // pointer-based dragging (works with mouse & touch via pointer events)
  let dragging = false;
  let startX = 0, startY = 0, origRight = null, origBottom = null;

  function onPointerDown(evt) {
    // only start drag when primary button / touch
    if (evt.button !== undefined && evt.button !== 0) return;
    dragging = true;
    startX = evt.clientX;
    startY = evt.clientY;
    const rect = bubble.getBoundingClientRect();
    // store current offset from right/bottom (prefer explicit style if present)
    origRight = parseFloat(getComputedStyle(bubble).right || (window.innerWidth - rect.right)) || (window.innerWidth - rect.right);
    origBottom = parseFloat(getComputedStyle(bubble).bottom || (window.innerHeight - rect.bottom)) || (window.innerHeight - rect.bottom);
    bubble.style.transition = 'none';
    try { evt.target.setPointerCapture && evt.target.setPointerCapture(evt.pointerId); } catch(e){}
  }
  function onPointerMove(evt) {
    if (!dragging) return;
    const dx = startX - evt.clientX;
    const dy = startY - evt.clientY;
    const newRight = Math.max(8, Math.round(origRight + dx));
    const newBottom = Math.max(8, Math.round(origBottom + dy));
    bubble.style.right = `${newRight}px`;
    bubble.style.bottom = `${newBottom}px`;
    // when dragging, keep panel closed for predictability
    if (expanded) setExpanded(false);
  }
  function onPointerUp(evt) {
    if (!dragging) return;
    dragging = false;
    bubble.style.transition = ''; // restore CSS transition
    try { evt.target.releasePointerCapture && evt.target.releasePointerCapture(evt.pointerId); } catch(e){}
  }

  handle.addEventListener('pointerdown', onPointerDown, { passive: true });
  window.addEventListener('pointermove', onPointerMove, { passive: true });
  window.addEventListener('pointerup', onPointerUp, { passive: true });

  // double-click resets to default docked position
  handle.addEventListener('dblclick', (e) => {
    bubble.style.right = '24px';
    bubble.style.bottom = '140px';
  });

  // ensure consistent initial state
  setExpanded(false);
})();

// add prev/next wiring (safe fallback uses aside links or hash)
{
  (function () {
    const prevBtn = document.getElementById('prevBtn');
    const nextBtn = document.getElementById('nextBtn');
    const bubblePrev = document.getElementById('bubblePrev');
    const bubbleNext = document.getElementById('bubbleNext');

    function getArticles() {
      return Array.from(document.querySelectorAll('.article-stage article[id]'));
    }

    function navigate(offset) {
      const arts = getArticles();
      if (!arts.length) return;
      let idx = arts.findIndex(a => a.classList.contains('active'));
      if (idx === -1) idx = 0;
      const nextIndex = (idx + offset + arts.length) % arts.length;
      const id = arts[nextIndex].id;
      // prefer SPA helper if available
      if (typeof window.hellishShowArticle === 'function') {
        window.hellishShowArticle(id);
      } else {
        const link = document.querySelector(`.doc-nav a[href="#${id}"]`);
        if (link) link.click();
        else location.hash = `#${id}`;
      }
    }

    if (prevBtn) prevBtn.addEventListener('click', (e) => { e.preventDefault(); navigate(-1); });
    if (nextBtn) nextBtn.addEventListener('click', (e) => { e.preventDefault(); navigate(1); });
    if (bubblePrev) bubblePrev.addEventListener('click', (e) => { e.preventDefault(); navigate(-1); });
    if (bubbleNext) bubbleNext.addEventListener('click', (e) => { e.preventDefault(); navigate(1); });
  })();
}