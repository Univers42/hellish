# hellish AI

An optional AI layer is **built into every `make` build** — no special flag.
It is **on by default in interactive sessions** (and off for scripts / `-c` /
pipes, so non-interactive use and the test suite are never affected). Turn it
off any time:

```sh
set +o ai      # off        set -o ai   # back on        ai status   # what's on
```

Everything degrades gracefully: with no model server reachable, the only live
feature is history ghost-text (which needs no AI); the rest silently does
nothing until you give it a backend.

## What you get

| Feature | How to use |
|---|---|
| **Ghost-text** | As you type, the most recent matching command from history shows dim; press **→** to accept. Instant, local, no server needed. |
| **Inline AI completion** | Type a partial command, press **Ctrl-X Ctrl-A** — the model rewrites the line into a full command. |
| **Ask** | `ai ask <question>` — a normal answer. |
| **Do** | `ai do <task>` — returns just a runnable shell command (markdown stripped). |
| **Pro-tip line** | A shell tip appears above the prompt, refreshed in the background (never blocks the prompt). |
| **Model picker** | `ai model` opens a fuzzy picker over the server's models; `ai model <name>` sets one directly. |
| **Status** | `ai status` shows the toggle + the provider chain. |
| **Setup** | `ai setup <provider>` writes the config to `~/.hellishrc` (see below). |

`ai` commands work even when `set +o ai` (the toggle only gates the passive
features — ghost-text, pro-tips, inline completion).

## Backends

hellish speaks the OpenAI-compatible HTTP API, so it works with a local
llama.cpp server **or** any cloud provider. The chain is: a configured **cloud
primary**, then automatic **fallback to the local server** on any error or
rate-limit (HTTP 429). Pick what fits:

### Local (private, offline, free)

A llama.cpp `llama-server` in Docker, reached at `127.0.0.1:8080`:

```sh
make ai-pull MODEL=<https url to a .gguf>   # once: download a model into the volume
make ai-up                                  # start it (CPU). Stop with: make ai-down
```

Choose how it runs — `make ai-up COMPUTE=cpu|hybrid|gpu` (GPU is vendor-agnostic
via Vulkan: AMD/Intel/NVIDIA). Trade-offs and tuning: **[AI-COMPUTE.md](AI-COMPUTE.md)**.

A small model (e.g. Qwen2.5-0.5B/3B) is fine for `ai do` / completion; bigger
models answer better but need more RAM/VRAM.

### Cloud (fastest + smartest)

Run the helper — it prompts for your key (hidden) and writes `~/.hellishrc`:

```sh
ai setup groq          # free, very fast (Llama-3.3-70B). Key: console.groq.com
ai setup openrouter    # Claude & many models       Key: openrouter.ai
ai setup openai        # gpt-4o-mini, …              Key: platform.openai.com
ai setup local         # local-only (no cloud)
```

Or configure by hand in `~/.hellishrc` (any OpenAI-compatible API):

```sh
export HELLISH_AI_URL=https://api.groq.com/openai/v1/chat/completions
export HELLISH_AI_KEY=gsk_your_key_here
export HELLISH_AI_MODEL=llama-3.3-70b-versatile
```

Then `source ~/.hellishrc` (or restart). A cloud primary still falls back to a
running local server if it errors.

## Environment variables

| Var | Meaning | Default |
|---|---|---|
| `HELLISH_AI_URL` | Cloud chat-completions endpoint (unset = local only) | — |
| `HELLISH_AI_KEY` | Bearer API key for the cloud endpoint | — |
| `HELLISH_AI_MODEL` | Model id sent to the cloud endpoint | — |
| `HELLISH_AI_HOST` | Local server host | `127.0.0.1` |
| `HELLISH_AI_PORT` | Local server port | `8080` |
| `HELLISH_AI_TIMEOUT_MS` | Per-request timeout | `20000` |

## Privacy & cost

A **cloud** backend sends your request — including the injected shell context
(cwd, OS, git branch, recent commands, current directory listing) — to that
provider, and may cost money per their pricing. A **local** backend keeps
everything on your machine. The context is what lets the model understand
intent; it never includes file *contents*. Use `ai setup local` (or just don't
set `HELLISH_AI_URL`) to stay fully local.

## Notes

- Transport: cloud requests use `curl` (for TLS + auth); local uses a built-in
  socket client (no curl needed offline).
- Ghost-text is history-based (like fish) — instant and never hits the network.
  The LLM powers the explicit Ctrl-X Ctrl-A completion instead.
- Building the LLM stack needs Docker; the shell itself needs nothing extra.
