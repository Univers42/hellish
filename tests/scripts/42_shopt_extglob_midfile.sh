#!/bin/bash
# `shopt -s extglob` must arm the lexer for LATER lines of the same input
# (bash reads and runs incrementally, so line N+1 lexes with whatever
# line N set). hellish tokenized ahead of execution, and bash-completion
# arms the option on its line 47 then relies on it ~1800 lines further
# down: every ssh login on a Debian 13 box died with "syntax error near
# unexpected token `('" before ~/.profile finished (issue #105).
# NOTE: the lines above the first case must stay free of batch-hazard
# words; a stray one would split the batch here and mask the bug.
shopt -s extglob
case foo in
	f@(oo)) echo "case-extglob: matched" ;;
	*) echo "case-extglob: MISSED" ;;
esac
case "ab" in
	+(a|b)) echo "plus-group: matched" ;;
	*) echo "plus-group: MISSED" ;;
esac
case "zz" in
	!(a*)) echo "negate-group: matched" ;;
	*) echo "negate-group: MISSED" ;;
esac

# The same rule through the dot builtin: the file read there arms the
# option on ITS first line and uses it below.
cat > sub_extglob.sh <<'EOF'
shopt -s extglob
case "docs" in
	d?(ocs)) echo "dotted-extglob: matched" ;;
	*) echo "dotted-extglob: MISSED" ;;
esac
EOF
. ./sub_extglob.sh
rm -f sub_extglob.sh

# And through eval, across a newline (bash arms it for the eval string's
# later statements too).
eval 'shopt -s extglob
case wow in w@(ow)) echo "eval-extglob: matched" ;; *) echo "eval-extglob: MISSED" ;; esac'
