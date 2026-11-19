# hellish-shell

**polish** (formerly hellish) — a fast, POSIX-compliant shell. 🔥

The package name and the `hellish` command are unchanged since the rename.

```sh
npm  install -g hellish-shell
pnpm add     -g hellish-shell
yarn global add hellish-shell
```

Then run it:

```sh
hellish
```

On install, a small `postinstall` step downloads the matching prebuilt binary
from the project's [GitHub Releases](https://github.com/Univers42/polish/releases).
Currently ships a prebuilt binary for **linux / x64**; on other platforms,
[build from source](https://github.com/Univers42/polish).

## Updating

From inside the shell:

```sh
update          # check for a newer release
update --now    # download & install the latest binary
```

## Links

- Source & issues: https://github.com/Univers42/polish
- Docker image: `docker run --rm -it <login>/hellish-shell`

MIT licensed.
