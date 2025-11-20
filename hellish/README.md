# Website

This website is built using [Docusaurus](https://docusaurus.io/), a modern static website generator.

## Installation

```bash
yarn
```

## Local Development

```bash
yarn start
```

This command starts a local development server and opens up a browser window. Most changes are reflected live without having to restart the server.

## Build

```bash
yarn build
```

This command generates static content into the `build` directory and can be served using any static contents hosting service.

## Deployment

Using SSH:

```bash
USE_SSH=true yarn deploy
```

Not using SSH:

```bash
GIT_USER=<Your GitHub username> yarn deploy
```

If you are using GitHub pages for hosting, this command is a convenient way to build the website and push to the `gh-pages` branch.

## Architecture of app

- docs:
  - content managed by the docs plugin. Markdown/MDX files become documentation pages.
  - Use front matter (---) to set title, sidebar_label, posititon, slug, etc.
  - Example: /docs/hello (or custom slug)
  - sidebar is generated from this folder (or controlled via sidebar.ts)
- blog:
  - blog posts (markdown/MDX). Each file becomes a blog post page and indexes/tags/RSS are generated.
  - Example: blog/2021-05-01-post.md → /blog/2021/05/01/post or /blog/post-slug.
- src:
  - Client-side React code, custom pages and components.
  - Src/pages/\* -> routes (src/pages/index.tsx -> /src/pages/foo.md -> /foo).
  - src/components/ -> reusable UI components; src/css/ -> custome styles
  - code here is bundled by the site (runs in the browser)
- static:
  - static assets copies as-is into the final build root. Accessible by absolute paths.
  - Example: static/img/logo.png -> /img/logo.png in the site.
  - Use for images, robots.txt, favicon,etc..

# Hellish — Project Documentation

This repo contains the documentation site for the "hellish" project (Docusaurus).

Quick commands:
- npm install
- npm run start    # run local dev server (http://localhost:3000)
- npm run build    # build static site
- npm run serve    # preview built site

The homepage is customized at `src/pages/index.tsx`. Docs are under `docs/`. Edit `src/css/custom.css` to change styles and animations.
