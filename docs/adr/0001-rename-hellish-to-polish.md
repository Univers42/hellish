# 0001. Rename the product to polish without breaking any hellish install

- Status: accepted
- Date: 2026-11-19
- Deciders: the maintainer. Everyone with hellish ≤ 3.3.0 installed has to live with it.

## Context

- The GitHub repository is already `Univers42/polish`. The old name `Univers42/hellish`
  works only through GitHub's rename redirect. `curl -fsSL
  https://api.github.com/repos/Univers42/hellish/releases/latest` returned `tag_name:
  "v3.3.0"` on 2026-11-19.
- Every installed binary up to 3.3.0 updates itself by querying that old URL, downloading the asset
  `hellish-linux-<uname -m>`, and accepting the download only when the text after the first space of
  `<new> -c "update --version"` equals the tag minus `v` (`validate_binary`/`parse_tag`,
  `src/platform/posix/update_apply.c` at v3.3.0). That code is frozen in the field.
- Users have configuration on disk under the old name: `~/.hellishrc`,
  `$XDG_CONFIG_HOME/hellish/`, `/etc/hellish/`, `HELLISH_*` knobs and hook arrays in their rc
  files, `$HELLISH_GIT_*` in their PS1, and `*.hellish`/`*.hell` scripts.
- GitHub Pages does not redirect: `univers42.github.io/hellish/whats-new/` returns 404 and
  `/polish/whats-new/` returns 200. The link printed by old banners is dead, and nothing in
  this repository can fix it.

## Decision

3.4.0 is named polish everywhere a new user can see it. Every name an existing user already
depends on keeps working:

- `src/brand/` is the only place where the two names meet.
  - `brand_pick` uses the new path when it exists, or when the legacy path does not exist;
    otherwise it uses the legacy path. This covers the rc file, the XDG directory and `/etc`.
  - `brand_getenv`, `brand_env` and `brand_expand` read `POLISH_X` if it is set at all,
    otherwise `HELLISH_X`. It is never both, so a hook array runs once.
- `GIT_*` prompt variables are published under both prefixes.
- `update --version` prints `polish <bare version>`. `tests/brand_compat_test.py` replays the
  old `validate_binary` rule against it.
- Every release publishes `hellish-linux-x86_64` (+ `.sha256`) with the same bytes as
  `polish-linux-x86_64`. The release job fails if any of the four files is missing.
- `build/bin/hellish` is a symlink to `build/bin/polish`.
- Installers keep the existing hellish paths when they detect an existing install.
- **No repository named `Univers42/hellish` may ever be created.** It would replace the redirect,
  and every ≤ 3.3.0 install would silently stop updating.

## Alternatives considered

- **Rename everything, with no fallbacks.** An update would drop the user's `~/.hellishrc`,
  their hooks and their prompt variables on the floor. That is exactly what the user asked to avoid.
- **Keep reading both rc files.** Both would run, so hook arrays would duplicate and an rc
  that appends to `PATH` would append twice. First-match-wins is the only shape where both
  files can exist without surprises.
- **`update --version` prints `polish v3.4.0`.** The old updater would compare `v3.4.0` with
  `3.4.0`, reject the download, and leave every old install stuck on 3.3.0 for good.
- **Rename the npm package and the Docker Hub image too.** Both need a new registry name plus a
  deprecation path, and the shell does not depend on either. Deferred.

## Consequences

- The `hellish-linux-x86_64` asset, the `Univers42/hellish` redirect and the
  `<word> <version>` shape of `update --version` are permanent public contracts.
  `make brand-test` guards the last one.
- A ≤ 3.3.0 updater installs 3.4 as `.../hellish` and adds no `polish` command. Since 3.4.1,
  the binary adds the `polish -> hellish` symlink itself (`src/platform/posix/update_brand.c`,
  `make legacy-update-test`).
- `src/brand/` and the legacy branches in the installers can be removed only when no ≤ 3.3.0
  install is left, and nothing measures that. Treat it as never.
- Internal identifiers (`hellish_version_cmp`, `HELLISH_VERSION`, file names like
  `seed_hellishrc`) keep the old name. They are not user-visible, and renaming them would conflict
  with every open branch.
