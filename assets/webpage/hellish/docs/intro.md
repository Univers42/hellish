---
id: intro
title: Project overview
---

Welcome to the Hellish / sh42 documentation.

This site gathers everything you need to:

- Quickly start using sh42 (Quick start).
- Read full documentation (Architecture, CLI, API, Examples).
- Find team and contribution details.

Navigation:

- Getting started — a short path to running and testing locally.
- Full docs — guides, API references and examples.

Goals

- Document the project structure and how to run the documentation site.
- Provide onboarding notes for collaborators.
- Keep a short reference for blog posts, authors and tags already present in this repo.

Where to look

- Blog posts are in the `/blog` folder (already present).
- Author and tag metadata lives in `/blog/authors.yml` and `/blog/tags.yml`.

---

## sidebar_position: 1

# Tutorial Intro

Let's discover **Docusaurus in less than 5 minutes**.

## Getting Started

Get started by **creating a new site**.

Or **try Docusaurus immediately** with **[docusaurus.new](https://docusaurus.new)**.

### What you'll need

- [Node.js](https://nodejs.org/en/download/) version 20.0 or above:
  - When installing Node.js, you are recommended to check all checkboxes related to dependencies.

## Generate a new site

Generate a new Docusaurus site using the **classic template**.

The classic template will automatically be added to your project after you run the command:

```bash
npm init docusaurus@latest my-website classic
```

You can type this command into Command Prompt, Powershell, Terminal, or any other integrated terminal of your code editor.

The command also installs all necessary dependencies you need to run Docusaurus.

## Start your site

Run the development server:

```bash
cd my-website
npm run start
```

The `cd` command changes the directory you're working with. In order to work with your newly created Docusaurus site, you'll need to navigate the terminal there.

The `npm run start` command builds your website locally and serves it through a development server, ready for you to view at http://localhost:3000/.

Open `docs/intro.md` (this page) and edit some lines: the site **reloads automatically** and displays your changes.
