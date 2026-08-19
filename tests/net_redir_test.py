#!/usr/bin/env python3
"""Regression test: /dev/tcp and /dev/udp virtual redirections (issue #16).

These paths exist on no filesystem -- a shell either intercepts them and
hands back a connected socket, as bash and ksh do, or it calls open(2) and
reports "No such file or directory", which is what hellish used to do.

The golden suite covers every case that needs no peer (refused, bad host,
bad service, a path that only LOOKS like one of these). The cases here are
the ones that need something listening, so this driver brings up its own
TCP and UDP servers on an ephemeral port and diffs hellish against bash
across them: reading, writing, a bidirectional exchange on one fd, `<` and
`>` and `<>`, a hostname rather than a literal address, and the liveness
probe idiom from the issue.

bash is the specification here, and the same oracle pin as tests/tester
applies: $HOME/bash-5.3.9 when it exists, PATH bash otherwise, with a
warning, because bash's behaviour here is not identical across releases.

Usage: python3 net_redir_test.py /path/to/hellish
"""
import os
import socket
import subprocess
import sys
import threading

FAILS = []


def check(name, ok, detail=""):
    print("  %-38s %s%s" % (name, "ok" if ok else "FAIL",
                            "" if ok else "   " + detail))
    if not ok:
        FAILS.append(name)


class Peer(object):
    """A TCP acceptor and a UDP sink sharing one port number.

    The TCP side greets every connection with "HELLO" and then reads
    whatever the client sends, so a script can test read-only, write-only
    and bidirectional use of the same fd. The UDP side just drains, which
    is all a datagram redirection needs to look successful.
    """

    def __init__(self):
        self.tcp = socket.socket()
        self.tcp.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.tcp.bind(("127.0.0.1", 0))
        self.port = self.tcp.getsockname()[1]
        self.tcp.listen(16)
        self.tcp.settimeout(0.25)
        self.udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.udp.bind(("127.0.0.1", self.port))
        self.udp.settimeout(0.25)
        self.stop = threading.Event()
        self.threads = [threading.Thread(target=self._tcp),
                        threading.Thread(target=self._udp)]
        for t in self.threads:
            t.daemon = True
            t.start()

    def _tcp(self):
        while not self.stop.is_set():
            try:
                conn, _ = self.tcp.accept()
            except Exception:
                continue
            try:
                conn.sendall(b"HELLO\n")
                conn.settimeout(0.25)
                try:
                    conn.recv(4096)
                except Exception:
                    pass
            finally:
                conn.close()

    def _udp(self):
        while not self.stop.is_set():
            try:
                self.udp.recvfrom(4096)
            except Exception:
                pass

    def close(self):
        self.stop.set()
        for t in self.threads:
            t.join(timeout=1.0)
        self.tcp.close()
        self.udp.close()


def run(shell, script):
    """(stdout, exit status) for one -c script. stderr is dropped: the
    project does not diff error wording, only status and output."""
    p = subprocess.run(shell + ["-c", script], stdout=subprocess.PIPE,
                       stderr=subprocess.DEVNULL, timeout=25)
    return p.stdout.decode("utf-8", "replace"), p.returncode


def oracle():
    pinned = os.path.join(os.path.expanduser("~"), "bash-5.3.9", "bin", "bash")
    if os.access(pinned, os.X_OK):
        return [pinned, "--posix"]
    print("warning: pinned bash 5.3.9 not found; grading against PATH bash "
          "(run 'make oracle' for the pin)")
    return ["bash", "--posix"]


def main():
    if len(sys.argv) < 2:
        print("usage: net_redir_test.py /path/to/hellish")
        sys.exit(2)
    hellish = [os.path.abspath(sys.argv[1])]
    bash = oracle()
    env = dict(os.environ, HELLISH_NO_BANNER="1", HELLISH_NO_ANIM="1")
    os.environ.update(env)

    peer = Peer()
    p = peer.port
    cases = [
        ("open <> succeeds", "exec 3<>/dev/tcp/127.0.0.1/%d" % p),
        ("read from socket", "exec 3<>/dev/tcp/127.0.0.1/%d; head -1 <&3" % p),
        ("bidirectional on one fd",
         "exec 3<>/dev/tcp/127.0.0.1/%d; printf 'ping\\n' >&3; head -1 <&3" % p),
        ("read-only <", "exec 3</dev/tcp/127.0.0.1/%d; head -1 <&3" % p),
        ("write-only >",
         "exec 3>/dev/tcp/127.0.0.1/%d; echo hi >&3; echo wrote" % p),
        ("stdin redirect", "head -1 < /dev/tcp/127.0.0.1/%d" % p),
        ("hostname not literal address",
         "exec 3<>/dev/tcp/localhost/%d; head -1 <&3" % p),
        ("udp datagram",
         "exec 3<>/dev/udp/127.0.0.1/%d; echo hi >&3; echo udp-ok" % p),
        ("liveness probe, host up",
         "(exec 3<>/dev/tcp/127.0.0.1/%d) && echo UP || echo DOWN" % p),
        ("liveness probe, host down",
         "(exec 3<>/dev/tcp/127.0.0.1/1) 2>/dev/null && echo UP || echo DOWN"),
        ("fd survives into a later command",
         "exec 3<>/dev/tcp/127.0.0.1/%d; true; head -1 <&3" % p),
        ("two sockets at once",
         "exec 3<>/dev/tcp/127.0.0.1/%d; exec 4<>/dev/tcp/127.0.0.1/%d; "
         "head -1 <&3; head -1 <&4" % (p, p)),
        ("closing the fd", "exec 3<>/dev/tcp/127.0.0.1/%d; exec 3>&-; "
                           "echo closed" % p),
        ("socket in a pipeline",
         "exec 3<>/dev/tcp/127.0.0.1/%d; head -1 <&3 | tr a-z A-Z" % p),
    ]
    print("== /dev/tcp and /dev/udp redirections (peer on port %d) ==" % p)
    try:
        for name, script in cases:
            ho, hc = run(hellish, script)
            bo, bc = run(bash, script)
            check(name, (ho, hc) == (bo, bc),
                  "hellish=%r/%d bash=%r/%d" % (ho, hc, bo, bc))
    finally:
        peer.close()

    print("== %s ==" % ("ALL PASSED" if not FAILS else
                        "%d FAILURES: %s" % (len(FAILS), ", ".join(FAILS))))
    sys.exit(1 if FAILS else 0)


if __name__ == "__main__":
    main()
