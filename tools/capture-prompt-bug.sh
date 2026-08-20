#!/usr/bin/env bash
# Capture the raw terminal byte stream while you use hellish normally, so an
# intermittent prompt corruption can be diagnosed from ground truth instead
# of from a screenshot.
#
# On screen the corruption looks like one of these:
#   ╰─❯ 8;2;90;96;106m╰─❯     an escape that lost its ESC[ introducer
#   ╰─❯ <?>                   a UTF-8 character cut mid-sequence
#
# Usage:  tools/capture-prompt-bug.sh
#         ... use the shell normally until you see the corruption ...
#         exit
# Then send the .analysis file it prints.
set -u
out="${TMPDIR:-/tmp}/hellish-prompt-capture.$$"
bin="${HELLISH:-$(command -v hellish || echo ./build/bin/hellish)}"

echo "recording to $out.raw   (shell: $bin)"
echo "reproduce the corruption, then type 'exit'."
# script(1) records everything the terminal received, escape bytes included.
script -q -f -c "$bin" "$out.raw"

{
	echo "=== capture analysis ==="
	echo "binary : $bin"
	echo "TERM=${TERM:-unset} COLORTERM=${COLORTERM:-unset} LANG=${LANG:-unset}"
	echo "size   : $(stat -c%s "$out.raw" 2>/dev/null) bytes"
	echo
	echo "--- SGR bodies that reached the screen without their ESC[ ---"
	grep -aoP '(?<!\x1b\[)(?<![\x1b\[0-9;])\b\d{1,3}(?:;\d{1,3}){2,}m' \
		"$out.raw" 2>/dev/null | sort | uniq -c | head -20
	echo
	echo "--- replacement characters (cut UTF-8), line count ---"
	grep -ac $'\xef\xbf\xbd' "$out.raw" 2>/dev/null
	echo
	echo "--- context around the first orphan ---"
	perl -0777 -ne 'if (/((?<!\x1b\[)\b\d{1,3}(?:;\d{1,3}){2,}m)/) {
		my $p = $-[0]; my $s = $p > 150 ? $p-150 : 0;
		my $c = substr($_, $s, 300); $c =~ s/\x1b/<ESC>/g;
		print "byte offset $p\n$c\n"; }' "$out.raw" 2>/dev/null
} > "$out.analysis" 2>&1

echo
echo "analysis : $out.analysis"
echo "raw      : $out.raw"
