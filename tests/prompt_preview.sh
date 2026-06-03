#!/bin/sh
# Preview every experimental hellish prompt style.
# Usage:  sh tests/prompt_preview.sh
# Then try one live:  HELLISH_PROMPT_STYLE=wave ./build/bin/hellish
#
# Styles (set $HELLISH_PROMPT_STYLE):
#   glow       (default) a calm two-line prompt: a breathing star ✦, your
#              context, and a warm ray of light that glides along the rule
#   breathe    a single softly-pulsing sparkle on a calm, still line
#   aurora     braille spinner + a colour gradient that flows along the rule
#   wave       a live audio-waveform that scrolls
#   pulse      ultra-minimal one line: a breathing orb + cwd + arrow
#   powerline  sleek angled bg segments (needs a Nerd Font for the  glyph)
#   classic    the original rounded box prompt
# The multi-line styles animate IN PLACE every ~90ms via a readline idle hook,
# and KEEP animating while you type -- the hook works out how far the decorative
# line is above the (possibly wrapped) cursor, repaints it, and restores the
# cursor, so the input line is never touched. Arrow is green/red by $?; beam.
# NOTE: this preview only shows the static prompt -- run it live to see motion.

H="$(CDPATH= cd "$(dirname "$0")/.." && pwd)/build/bin/hellish"
[ -x "$H" ] || { echo "build first: make OPT=1 all" >&2; exit 1; }
COLS="${COLUMNS:-90}"

for style in glow breathe aurora wave pulse powerline classic; do
	printf '\n\033[1m──────── %s ────────\033[0m\n' "$style"
	# drive a real prompt through a pty, run two commands so a frame advances,
	# then show the prompt lines (strip the bracketed-paste guards).
	printf 'true\nfalse\nexit\n' \
		| HELLISH_PROMPT_STYLE="$style" COLUMNS="$COLS" \
			script -qec "$H" /dev/null 2>/dev/null \
		| sed 's/\r//g; s/\[?2004[hl]//g' \
		| grep -aE '❯|✦|─|▁|▂|▃|▄|▅|▆|▇|█|○|◑|◔|●|⠋|⠙|⠹|⠸|⠼|in |·| ' \
		| grep -av '^true$\|^false$\|^exit$' \
		| head -4
done
printf '\n\033[2mTry one live:  HELLISH_PROMPT_STYLE=glow %s\033[0m\n' "$H"
