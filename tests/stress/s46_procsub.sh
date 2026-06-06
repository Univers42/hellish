#!/bin/sh
# Process substitution: compare two generated streams and read from one, all
# without temporary files.
diff <(seq 1 5) <(seq 1 2; seq 4 6) || true
echo "---"
while read -r a; do printf '%s ' "$a"; done < <(printf 'x\ny\nz\n')
echo
