#!/usr/bin/env python3
"""AI subsystem regression tests for hellish.

Covers the four things that were broken and repaired on feat/ai:
  * provider abstraction + system prompt   (request shape, auth, reply parsing)
  * prompt latency                          (no network on the REPL hot path)
  * prompt display on resize                (arrow does not duplicate on zoom)
  * history navigation                      (arrow does not duplicate)

Provider + latency tests use only the standard library. The resize/navigation
tests need `pyte` (a terminal emulator); they are skipped with a notice if it
is not installed, so this stays runnable in a bare CI. Usage:

    python3 tests/ai_test.py ./build/bin/hellish
"""
import json
import os
import re
import select
import socket
import struct
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
BIN = sys.argv[1] if len(sys.argv) > 1 else "./build/bin/hellish"
BIN = os.path.abspath(BIN)
HOME = os.path.join(HERE, ".ai_test_home")
BLACKHOLE = "192.0.2.1"          # RFC5737 TEST-NET-1: drops SYN, never connects
PASS, FAIL = [], []


def ok(name):
    PASS.append(name)
    print(f"  ok   {name}")


def bad(name, why):
    FAIL.append(name)
    print(f"  FAIL {name}: {why}")


def free_port():
    s = socket.socket()
    s.bind(("127.0.0.1", 0))
    p = s.getsockname()[1]
    s.close()
    return p


def fresh_home():
    os.makedirs(os.path.join(HOME, ".cache/hellish"), exist_ok=True)
    open(os.path.join(HOME, ".cache/hellish/seen"), "w").close()
    for junk in (".cache/hellish/protip", ".minishell_history"):
        try:
            os.remove(os.path.join(HOME, junk))
        except FileNotFoundError:
            pass


# --------------------------------------------------------------------------- #
# Provider / system-prompt tests (stdlib only, via the fake LLM server)
# --------------------------------------------------------------------------- #
def start_fake(port, log, reply="echo FAKED", delay="0"):
    env = dict(os.environ, FAKE_LLM_PORT=str(port), FAKE_LLM_LOG=log,
               FAKE_LLM_REPLY=reply, FAKE_LLM_DELAY=delay)
    p = subprocess.Popen([sys.executable, os.path.join(HERE, "ai_fake_llm.py")],
                         env=env, stdout=subprocess.DEVNULL,
                         stderr=subprocess.DEVNULL)
    for _ in range(50):
        try:
            socket.create_connection(("127.0.0.1", port), 0.1).close()
            return p
        except OSError:
            time.sleep(0.05)
    return p


def run_ai(extra_env, prompt='ai ask "hi"'):
    env = dict(os.environ, HOME=HOME)
    env.update(extra_env)
    r = subprocess.run([BIN, "-c", prompt], env=env, capture_output=True,
                       text=True, timeout=15)
    return r.stdout


def last_log(log):
    lines = [l for l in open(log)] if os.path.exists(log) else []
    return json.loads(lines[-1]) if lines else None


def test_providers():
    fresh_home()
    port = free_port()
    log = os.path.join(HOME, "req.log")
    open(log, "w").close()
    srv = start_fake(port, log, reply="echo FAKED")
    base = f"http://127.0.0.1:{port}"
    try:
        # local (native socket) path: system prompt present, reply parsed
        out = run_ai({"HELLISH_AI_HOST": "127.0.0.1", "HELLISH_AI_PORT": str(port)})
        rec = last_log(log)
        roles = [m["role"] for m in rec["body"].get("messages", [])]
        if "echo FAKED" in out:
            ok("local: assistant reply parsed")
        else:
            bad("local: assistant reply parsed", repr(out[:80]))
        if roles[:2] == ["system", "user"]:
            ok("local: system prompt sent")
        else:
            bad("local: system prompt sent", f"roles={roles}")

        # openai cloud path: Bearer auth, OpenAI shape
        open(log, "w").close()
        run_ai({"HELLISH_AI_URL": base + "/v1/chat/completions",
                "HELLISH_AI_KEY": "sk-openai"})
        rec = last_log(log)
        auth = rec["headers"].get("authorization", "")
        if auth.startswith("Bearer sk-openai"):
            ok("openai: Bearer auth header")
        else:
            bad("openai: Bearer auth header", auth[:20])
        if "system" not in rec["body"] and "stream" in rec["body"]:
            ok("openai: chat-completions shape")
        else:
            bad("openai: chat-completions shape", sorted(rec["body"]))

        # anthropic path: x-api-key + version, Messages shape (top-level system)
        open(log, "w").close()
        out = run_ai({"HELLISH_AI_URL": base + "/v1/messages",
                      "HELLISH_AI_PROVIDER": "anthropic",
                      "HELLISH_AI_KEY": "sk-ant"})
        rec = last_log(log)
        h = rec["headers"]
        if h.get("x-api-key") == "sk-ant" and \
                h.get("anthropic-version") == "2023-06-01":
            ok("anthropic: x-api-key + version headers")
        else:
            bad("anthropic: x-api-key + version headers", str(h.get("x-api-key")))
        b = rec["body"]
        roles = [m["role"] for m in b.get("messages", [])]
        if "system" in b and roles == ["user"] and "model" in b:
            ok("anthropic: Messages shape (top-level system)")
        else:
            bad("anthropic: Messages shape", f"keys={sorted(b)} roles={roles}")
        if "echo FAKED" in out:
            ok("anthropic: content-block reply parsed")
        else:
            bad("anthropic: content-block reply parsed", repr(out[:80]))
    finally:
        srv.terminate()


# --------------------------------------------------------------------------- #
# Completion request (stdlib pty): Ctrl-X Ctrl-A must send the tuned body
# (max_tokens 64, temperature 0, stop at newline, lite context w/ exit status)
# --------------------------------------------------------------------------- #
def test_completion_body():
    import pty
    fresh_home()
    port = free_port()
    log = os.path.join(HOME, "req.log")
    open(log, "w").close()
    srv = start_fake(port, log, reply="ls -la")
    env = dict(os.environ, HOME=HOME, TERM="xterm",
               HELLISH_AI_URL=f"http://127.0.0.1:{port}/v1/chat/completions",
               HELLISH_AI_KEY="sk-fake")
    pid, fd = pty.fork()
    if pid == 0:
        os.execve(BIN, [BIN, "-i"], env)
        os._exit(127)
    try:
        time.sleep(1.2)
        os.write(fd, b"true\n")          # sets last exit status = 0
        time.sleep(0.6)
        os.write(fd, b"ls -")
        time.sleep(0.4)
        os.write(fd, b"\x18\x01")        # C-x C-a: synchronous AI completion
        deadline = time.time() + 8
        rec = None
        while time.time() < deadline and rec is None:
            time.sleep(0.2)
            rec = last_log(log)
        os.write(fd, b"\x03exit\n")
        time.sleep(0.2)
        if rec is None:
            bad("completion: request sent on C-x C-a", "no request logged")
            return
        b = rec["body"]
        if b.get("max_tokens") == 64 and b.get("temperature") == 0 \
                and b.get("stop") == ["\n"]:
            ok("completion: tuned decoding (64 tok, temp 0, stop \\n)")
        else:
            bad("completion: tuned decoding",
                f"max_tokens={b.get('max_tokens')} temp={b.get('temperature')}"
                f" stop={b.get('stop')}")
        msgs = b.get("messages", [])
        sys_txt = msgs[0]["content"] if msgs else ""
        usr_txt = msgs[1]["content"] if len(msgs) > 1 else ""
        if "completion engine" in sys_txt:
            ok("completion: completion-tuned system prompt")
        else:
            bad("completion: completion-tuned system prompt", sys_txt[:60])
        if "last exit status: 0" in usr_txt:
            ok("completion: context carries last exit status")
        else:
            bad("completion: context carries last exit status", usr_txt[:120])
    finally:
        srv.terminate()
        try:
            os.close(fd)
        except OSError:
            pass


# --------------------------------------------------------------------------- #
# Latency test (stdlib pty): the REPL hot path must never block on AI network
# --------------------------------------------------------------------------- #
def test_latency():
    import pty
    fresh_home()
    env = dict(os.environ, HOME=HOME, HELLISH_AI_HOST=BLACKHOLE,
               HELLISH_AI_PORT="8080", HELLISH_AI_TIMEOUT_MS="800",
               TERM="xterm")
    env.pop("HELLISH_AI_URL", None)
    pid, fd = pty.fork()
    if pid == 0:
        os.execve(BIN, [BIN, "-i"], env)
        os._exit(127)
    ansi = re.compile(rb"\x1b\[[0-9;?]*[A-Za-z]|\x1b[=>]|\r")

    def drain(t):
        buf, end = b"", time.time() + t
        while time.time() < end:
            r, _, _ = select.select([fd], [], [], max(0, end - time.time()))
            if not r:
                continue
            try:
                d = os.read(fd, 65536)
            except OSError:
                break
            if not d:
                break
            buf += d
        return buf

    drain(2.0)
    worst = 0.0
    seq = 0
    for cmd in ["echo a", "clear", "pwd", "clear", "true", "clear"]:
        seq += 1
        token = f"zZ{seq}Zz"
        os.write(fd, f"{cmd} ; echo {token}\n".encode())
        t0, seen = time.time(), b""
        while time.time() - t0 < 6:
            seen += drain(0.05)
            if (b"\n" + token.encode() + b"\n") in ansi.sub(b"", seen):
                break
        worst = max(worst, time.time() - t0)
    os.write(fd, b"exit\n")
    time.sleep(0.2)
    try:
        os.close(fd)
    except OSError:
        pass
    if worst < 1.5:
        ok(f"latency: no REPL stall on blackholed backend (worst {worst:.2f}s)")
    else:
        bad("latency: no REPL stall", f"worst round-trip {worst:.2f}s")


# --------------------------------------------------------------------------- #
# Display tests (need pyte): resize + history navigation must not duplicate
# --------------------------------------------------------------------------- #
def _spawn_pty(rows, cols):
    import pty
    fresh_home()
    env = dict(os.environ, HOME=HOME, HELLISH_AI_HOST=BLACKHOLE,
               HELLISH_AI_PORT="8080", HELLISH_AI_TIMEOUT_MS="500",
               TERM="xterm-256color")
    env.pop("HELLISH_AI_URL", None)
    pid, fd = pty.fork()
    if pid == 0:
        os.execve(BIN, [BIN, "-i"], env)
        os._exit(127)
    import fcntl
    import termios
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", rows, cols, 0, 0))
    return fd


def _pump(fd, stream, t):
    end = time.time() + t
    while time.time() < end:
        r, _, _ = select.select([fd], [], [], max(0, end - time.time()))
        if not r:
            continue
        try:
            d = os.read(fd, 65536)
        except OSError:
            return
        if not d:
            return
        stream.feed(d)


def test_display():
    try:
        import pyte
    except ImportError:
        print("  skip resize/navigation tests (pyte not installed:"
              " pip install pyte)")
        return
    import fcntl
    import termios
    rows = 40
    # --- resize: arrow must not stack across zoom in/out ---
    fd = _spawn_pty(rows, 80)
    screen = pyte.Screen(80, rows)
    stream = pyte.ByteStream(screen)
    _pump(fd, stream, 2.0)
    os.write(fd, b"echo hello")
    _pump(fd, stream, 0.6)
    worst = 0
    edge_bad = None
    for cols in (40, 100, 30, 90, 50):
        screen.resize(rows, cols)
        fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", rows, cols, 0, 0))
        _pump(fd, stream, 0.6)
        worst = max(worst, max(l.count("❯") for l in screen.display))
        # responsive header: the box top row must end with ╮ exactly at the
        # new right edge (not wrapped, not short)
        tops = [l for l in screen.display if "╭─ " in l]
        if tops:
            row = tops[-1].rstrip()
            if len(row) != cols or not row.endswith("╮"):
                edge_bad = f"cols={cols} width={len(row)} end={row[-1:]!r}"
    os.write(fd, b"\x03exit\n")
    time.sleep(0.2)
    os.close(fd)
    if worst <= 1:
        ok("resize: prompt arrow not duplicated on zoom")
    else:
        bad("resize: prompt arrow not duplicated", f"{worst} arrows on one row")
    if edge_bad is None:
        ok("resize: header reflows to the exact new width")
    else:
        bad("resize: header reflows to the exact new width", edge_bad)
    # --- navigation: recalling history must not stack the arrow either ---
    fd = _spawn_pty(rows, 80)
    screen = pyte.Screen(80, rows)
    stream = pyte.ByteStream(screen)
    _pump(fd, stream, 1.5)
    for c in ("echo one", "echo two", "echo three"):
        os.write(fd, (c + "\n").encode())
        _pump(fd, stream, 0.3)
    for _ in range(6):
        os.write(fd, b"\x1b[A")
        _pump(fd, stream, 0.15)
    for _ in range(4):
        os.write(fd, b"\x1b[B")
        _pump(fd, stream, 0.15)
    _pump(fd, stream, 0.3)
    worst = max((l.count("❯") for l in screen.display), default=0)
    os.write(fd, b"\x03exit\n")
    time.sleep(0.2)
    os.close(fd)
    if worst <= 1:
        ok("navigation: prompt arrow not duplicated on up/down")
    else:
        bad("navigation: prompt arrow not duplicated", f"{worst} on one row")


def test_prediction():
    try:
        import pyte
    except ImportError:
        print("  skip prediction test (pyte not installed: pip install pyte)")
        return
    rows = 40
    fd = _spawn_pty(rows, 80)
    screen = pyte.Screen(80, rows)
    stream = pyte.ByteStream(screen)
    _pump(fd, stream, 1.5)
    # teach the pattern: `echo two` follows `echo one` (twice), then run
    # `echo one` again -- the empty prompt should ghost `echo two`.
    for c in ("echo one", "echo two", "echo one", "echo two", "echo one"):
        os.write(fd, (c + "\n").encode())
        _pump(fd, stream, 0.35)
    _pump(fd, stream, 0.5)
    prompt_rows = [l for l in screen.display if "❯" in l]
    cur = prompt_rows[-1] if prompt_rows else ""
    if "echo two" in cur:
        ok("prediction: empty prompt ghosts the likely next command")
    else:
        bad("prediction: empty prompt ghosts next command", repr(cur.strip()))
    # Enter on the ghosted empty line: the abandoned suggestion must NOT stay
    # in scrollback looking like a typed command.
    os.write(fd, b"\n")
    _pump(fd, stream, 0.5)
    rows = [l for l in screen.display if "❯" in l]
    prev = rows[-2] if len(rows) >= 2 else ""
    if "echo two" not in prev:
        ok("prediction: abandoned ghost erased on Enter")
    else:
        bad("prediction: abandoned ghost erased on Enter", repr(prev.strip()))
    twos = sum(1 for l in screen.display if l.strip() == "two")
    os.write(fd, b"\x1b[C")              # Right-arrow accepts the prediction
    _pump(fd, stream, 0.3)
    os.write(fd, b"\n")
    _pump(fd, stream, 0.5)
    now = sum(1 for l in screen.display if l.strip() == "two")
    os.write(fd, b"\x03exit\n")
    time.sleep(0.2)
    try:
        os.close(fd)
    except OSError:
        pass
    if now == twos + 1:
        ok("prediction: Right-arrow accepts and runs it")
    else:
        bad("prediction: Right-arrow accepts and runs it",
            f"'two' outputs {twos} -> {now}")


def test_multiline_ghost_safety():
    try:
        import pyte
    except ImportError:
        print("  skip multiline ghost test (pyte not installed)")
        return
    rows = 40
    fd = _spawn_pty(rows, 80)
    screen = pyte.Screen(80, rows)
    stream = pyte.ByteStream(screen)
    _pump(fd, stream, 1.5)
    # put a MULTI-LINE entry in history (unclosed quote -> continuation)
    os.write(fd, b'echo "hello\n')
    _pump(fd, stream, 0.4)
    os.write(fd, b'world"\n')
    _pump(fd, stream, 0.5)
    before = sum(l.count("hello") for l in screen.display)
    # typing a prefix of it must NOT ghost the multi-line entry: printing its
    # raw newlines desyncs the cursor and stacks shifted copies (`cho "hello`)
    os.write(fd, b"e")
    _pump(fd, stream, 0.6)
    os.write(fd, b"c")
    _pump(fd, stream, 0.6)
    after = sum(l.count("hello") for l in screen.display)
    corrupted = any('cho "hello' in l and 'echo "hello' not in l
                    for l in screen.display)
    os.write(fd, b"\x03exit\n")
    time.sleep(0.2)
    try:
        os.close(fd)
    except OSError:
        pass
    if not corrupted and after == before:
        ok("multiline history: never ghosted inline (no stacking)")
    else:
        bad("multiline history: never ghosted inline",
            f"corrupted={corrupted} hello {before}->{after}")


def test_idiom_prediction():
    try:
        import pyte
    except ImportError:
        print("  skip idiom prediction test (pyte not installed)")
        return
    rows = 40
    fd = _spawn_pty(rows, 80)
    screen = pyte.Screen(80, rows)
    stream = pyte.ByteStream(screen)
    _pump(fd, stream, 1.5)
    # fresh history has no personal bigram -> the general-usage idiom table
    # should predict `ls` after a cd
    os.write(fd, b"cd .\n")
    _pump(fd, stream, 0.6)
    prompt_rows = [l for l in screen.display if "❯" in l]
    cur = prompt_rows[-1].rstrip() if prompt_rows else ""
    os.write(fd, b"\x03exit\n")
    time.sleep(0.2)
    try:
        os.close(fd)
    except OSError:
        pass
    if cur.endswith("❯ ls"):
        ok("prediction: common-idiom fallback (cd -> ls)")
    else:
        bad("prediction: common-idiom fallback (cd -> ls)", repr(cur))


def test_multiline_recall_render():
    # Up-arrow on a multi-line entry must render REAL line breaks like bash,
    # not `^J` soup. Replacing rl_redisplay_function silently degrades
    # readline's multi-row rendering -- this guards the getc-wrapper design.
    import pty
    fresh_home()
    env = dict(os.environ, HOME=HOME, TERM="xterm-256color",
               HELLISH_AI_HOST=BLACKHOLE, HELLISH_AI_TIMEOUT_MS="500")
    env.pop("HELLISH_AI_URL", None)
    pid, fd = pty.fork()
    if pid == 0:
        os.execve(BIN, [BIN, "-i"], env)
        os._exit(127)

    def drain(t):
        buf, end = b"", time.time() + t
        while time.time() < end:
            r, _, _ = select.select([fd], [], [], 0.1)
            if r:
                try:
                    buf += os.read(fd, 65536)
                except OSError:
                    break
        return buf

    drain(1.5)
    os.write(fd, b'echo "aaa\n')
    drain(0.4)
    os.write(fd, b'bbb\n')
    drain(0.4)
    os.write(fd, b'ccc"\n')
    drain(0.6)
    os.write(fd, b"\x1b[A")
    out = drain(0.8)
    os.write(fd, b"\x03exit\n")
    time.sleep(0.2)
    try:
        os.close(fd)
    except OSError:
        pass
    if b"^J" not in out and b"bbb" in out:
        ok("multiline recall: real line breaks (bash parity, no ^J)")
    else:
        bad("multiline recall: real line breaks", repr(out[-80:]))


def main():
    if not os.path.exists(BIN):
        print(f"binary not found: {BIN}")
        sys.exit(2)
    print("== hellish AI subsystem tests ==")
    for t in (test_providers, test_completion_body, test_latency,
              test_display, test_prediction, test_multiline_ghost_safety,
              test_idiom_prediction, test_multiline_recall_render):
        try:
            t()
        except Exception as e:
            bad(t.__name__, f"{type(e).__name__}: {e}")
    print(f"\n{len(PASS)} passed, {len(FAIL)} failed")
    sys.exit(1 if FAIL else 0)


if __name__ == "__main__":
    main()
