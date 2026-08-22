# mk/help.awk — render `make help` from the Makefile's own annotations.
#
# A separate file rather than a make variable holding an awk program: that
# program has to survive make expansion, then shell word-splitting, then awk
# parsing, and $$0/$$1 plus nested quotes do not all come out the other side
# intact. It cost one "Unterminated quoted string" to find that out. A file is
# read by exactly one of those three, and can be linted on its own.
#
# Reads three annotations:
#   ##@ Section            heading
#   target: deps  ## text  target and description
#   ##! NAME=value  text   configurable variable
#
# Colours arrive as -v so the palette stays in mk/colors.mk, which already
# knows how to blank itself for NO_COLOR and non-tty output.

# No FS split for target lines. `FS = ":.*## "` looks right and is greedy, so
# it splits on the LAST "## " in the line -- a description that mentions "##"
# (this file documents an annotation syntax, so several do) came out truncated
# to its own tail. Found by `make help` printing "description (should print
# none)" as the text for help-targets. index() from the colon is unambiguous.

/^##@ / {
	printf "\n%s%s%s\n", head, substr($0, 5), reset
	next
}

# "NAME=value  description" -- split on the first DOUBLE space so a value may
# contain single spaces (OPT=1 SAFE=0) without being mistaken for prose.
/^##! / {
	line = substr($0, 5)
	gap = index(line, "  ")
	if (gap == 0) { name = line; desc = "" }
	else {
		name = substr(line, 1, gap - 1)
		desc = substr(line, gap + 2)
		sub(/^ +/, "", desc)
	}
	printf "  %s%-24s%s %s\n", var, name, reset, desc
	next
}

# A target line carrying a description. Anything without one is invisible
# here; `make help-targets` is what reports those.
/^[a-zA-Z0-9_.-]+:.*## / {
	colon = index($0, ":")
	rest = substr($0, colon + 1)
	mark = index(rest, "## ")
	if (mark == 0) next
	printf "  %s%-24s%s %s\n", tgt, substr($0, 1, colon - 1), reset, \
	       substr(rest, mark + 3)
}
