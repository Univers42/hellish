#!/bin/bash
# The Oils spec-test runner (test/sh_spec.py) and the spec/bin helpers are
# python2.  This ports the vendored copy in place so it runs on python3.13+.
# Idempotent.  Judgement semantics are untouched: the patches are mechanical
# (removed stdlib modules, bytes/str at the subprocess boundary, iteritems/
# xrange), plus one hellish-specific hunk: `--posix` is spelled `--posix`
# for hellish instead of `-o posix`.
set -eu

OILS="$(cd "$(dirname "$0")/../suites/oils" && pwd)"

# python3.13 removed `cgi`; sh_spec only uses cgi.escape (HTML output).
cat > "$OILS/test/cgi.py" <<'EOF'
from html import escape
EOF

python3 - "$OILS/test/sh_spec.py" <<'EOF'
import sys
p = sys.argv[1]
s = open(p).read()
if 'import io as cStringIO' not in s:
    s = s.replace('import cStringIO', 'import io as cStringIO')
    s = s.replace('p.stdin.write(code)', "p.stdin.write(code.encode('utf-8'))")
    s = s.replace("actual['stdout'], actual['stderr'] = p.communicate()",
                  "_out_b, _err_b = p.communicate()\n"
                  "            actual['stdout'] = _out_b.decode('utf-8', errors='replace')\n"
                  "            actual['stderr'] = _err_b.decode('utf-8', errors='replace')")
    s = s.replace('unique.iteritems()', 'unique.items()')
    s = s.replace('xrange(', 'range(')
    s = s.replace("json.loads(exp_json, encoding='utf-8')", 'json.loads(exp_json)')
    s = s.replace("""    try:
        s.decode('utf-8')
        return s  # it decoded OK
    except UnicodeDecodeError:
        return repr(s)  # ASCII representation""",
"""    if isinstance(s, bytes):
        try:
            return s.decode('utf-8')
        except UnicodeDecodeError:
            return repr(s)
    return s""")
    s = s.replace("""            # dash doesn't support -o posix
            if opts.posix and sh_label != 'dash':
                argv.extend(['-o', 'posix'])""",
"""            # dash doesn't support -o posix; hellish spells it --posix
            if opts.posix and sh_label == 'hellish':
                argv.append('--posix')
            elif opts.posix and sh_label != 'dash':
                argv.extend(['-o', 'posix'])""")
    open(p, 'w').write(s)
    print('sh_spec.py ported to python3')
else:
    print('sh_spec.py already ported')
EOF

sed -i '1s|python2|python3|' "$OILS"/spec/bin/*.py
chmod +x "$OILS"/spec/bin/*.py
