# Engineering Workflow & Code of Conduct

This is not a legal document; it is a **practical agreement** on how we work on
this repository so that the project stays:

- understandable,
- easy to review and debug,
- pleasant to extend over time.

If you open this project in 6 months, you should still be able to see **why** a
change was made and **how** it flows through the architecture.

---

## 1. Before You Write Code

Before touching the code, make sure you understand:

- the **high‑level design** (see `README.md` + module READMEs under `src/*`),
- the **git workflow** and commit rules below,
- how the **test / debug loops** work (`make`, debug flags, etc.).

Ask yourself:

> “Which module am I changing, and what is its contract?”

If the answer is fuzzy, read that module’s README first.

---

## 2. Git Workflow

### 2.1 Branches

- **`main`** (or `master`): always buildable, no broken state.
- **Feature / bug branches**: work in short‑lived branches named like:

  ```text
  feat/prompt-metadata
  fix/parser-heredoc
  refactor/lexer-tables
  ```

Never push unreviewed experiments directly to `main`.

### 2.2 Pull / Merge Requests

- Keep them **small and focused**: one logical topic.
- Include a short description:
  - what you changed,
  - why it was needed,
  - how you tested it.
- If you touch behavior (parser, expander, executor), add or update tests or
  at least some sample commands in the description.

### 2.3 Interactive Rebase Habits

Before merging/review:

- squash noisy commits (`fix typo`, `oops`, `final commit`) into meaningful
  ones,
- reorder commits if it makes the story clearer,
- ensure each commit compiles (no broken steps).

This keeps history *narrative*, not just a random log.

---

## 3. Commit Messages

Most teams converge on three pillars:

1. **Conventional structure** – consistent prefixes.
2. **Message clarity** – short subject, optional detailed body.
3. **One logical change per commit** – atomic and revertable.

### 3.1 Conventional prefixes

Use this table:

| type      | Meaning                                 |
|-----------|-----------------------------------------|
| `feat`    | New feature / capability                |
| `fix`     | Bug fix                                 |
| `docs`    | Documentation only                      |
| `style`   | Formatting, no logic changes            |
| `refactor`| Code change without feature/bug change  |
| `test`    | Add / modify tests                      |
| `chore`   | Tooling, build, CI, scripts, etc.       |

**Examples**

```bash
feat(prompt): add git branch and venv markers
fix(parser): handle nested heredocs correctly
docs(heredoc): explain tab-stripping behavior
refactor(lexer): extract operator table helpers
```

### 3.2 Imperative, not past tense

Use an imperative, present‑tense subject:

- `add parser for heredoc`   ✅
- `added parser for heredoc` ❌
- `fix path resolution`      ✅
- `fixed path resolution`    ❌

Try to stay under **50 characters** for the subject; longer details go in the
body, wrapped at around **72 characters**.

### 3.3 One logical change per commit

Each commit should answer:

> “If I revert this commit, do I revert exactly one idea?”

Examples of good commit boundaries:

- “implement process substitution parsing”
- “refactor prompt width calculation to use visible_width_cstr”
- “fix history encoding of multi‑line commands”

If the project tracks issues, reference them in the footer:

```text
Fixes: #24
Related: #31
```

---

## 4. Hooks, CI, and Safety Nets

### 4.1 Local Git hooks

You are encouraged to set up local hooks (in `.git/hooks/`) to catch issues
*before* they leave your machine, for example:

- `pre-commit` – run formatting / static checks.
- `commit-msg` – enforce commit prefix and length.
- `pre-push` – run a minimal test suite.

These hooks are not mandatory, but they are strongly recommended. If we add a
shared hooks script later, it will be documented here.

### 4.2 CI (GitHub Actions)

The CI (when configured) should, at minimum:

- build the project with the default `Makefile` target,
- run the core test suite (unit tests, script tests, etc.),
- optionally run static analysis (e.g., `-Wall -Wextra -Werror` builds).

A pull request should be considered **not ready** if CI is red.

---

## 5. Code Style & Architecture

### 5.1 Style

- Follow the existing C style in this repo (brace placement, naming, etc.).
- Keep functions small and single‑purpose where possible.
- Avoid clever macros where a simple helper function would do.
- Prefer explicit types and checks over magic numbers or hidden behavior.

If you add new code, make it look like it already belongs here.

### 5.2 Module boundaries

- Do not make random cross‑module calls.
- Ask: *“Does this belong to lexer / parser / expander / infra / helpers?”*
- If a piece of logic is reused across modules, consider moving it into
  `helpers/` or an appropriate shared submodule.

Respect the existing layering:

- Infrastructure → Lexer → Parser → Reparser / Expansion → Executor.

---

## 6. Documentation Expectations

- Every **module** should have a conceptual `README.md` (not just API docs).
- When you add a non‑trivial feature, update or create the relevant README in:
  - `src/<module>/README.md`, or
  - `assets/readmes/<topic>.md`.
- Document the **why** and **behavior**, not just the signatures.

If someone reading the docs cannot understand *how to use* or *extend* a
component, the documentation is not finished.

---

## 7. Feature Stack & Priorities

When adding features, try to respect this progression (roughly from bottom to
top):

1. **Core REPL**
   - Reliable input, prompt, history, and basic command execution.
2. **Command execution correctness**
   - Proper PATH lookup, environment handling, exit codes.
3. **Built‑ins**
   - `cd`, `exit`, `export`, `unset`, `type`, etc., with POSIX‑like behavior.
4. **Pipes & redirections**
   - Pipelines, file redirects, here‑documents, process substitution.
5. **Control flow & scripts**
   - `&&`, `||`, `;`, `&`, subshells, script execution, exit status
     propagation.
6. **Polish and ergonomics**
   - Autocompletion, aliases, profiles, configuration, prompt themes.

When in doubt, **fix correctness before adding convenience**.

---

## 8. People Conduct (Short Version)

Even in a small project, be kind:

- Review code with respect; criticise the code, not the person.
- Prefer clear explanations over sarcasm.
- Assume good intent; ask questions if something is unclear.

A healthy, respectful atmosphere is the fastest way to ship good software.

---

By contributing to this repository, you agree to follow these guidelines so
that `sh42` remains a project we are proud to work on and happy to return to.
