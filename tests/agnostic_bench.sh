#!/usr/bin/env bash
# ============================================================================
#  agnostic_bench.sh — the CROSS-SHELL speed matrix
#
#  The companion to tests/benchmark (which only races hellish vs bash --posix).
#  Here we throw the SAME portable workload at every shell we can find —
#  hellish, bash, dash, zsh, mksh, ksh, yash, busybox ash and fish — time each
#  best-of-N, and print:
#
#    1. a per-workload matrix    (rows = workloads, cols = shells, cells = ms,
#                                 the fastest shell in each row highlighted)
#    2. a fastest→slowest ranking (geomean across workloads) that says exactly
#       where hellish lands and which shells it beats / loses to.
#
#  Fairness rules:
#    * every shell runs the same command in its own NATURAL mode (no --posix
#      handicap) — the honest "which shell runs this fastest?" question.
#    * fish has its own syntax, so each workload carries a hand-written fish
#      translation; workloads without one simply skip fish.
#    * output-equality gate: bash's stdout is the oracle. A shell whose output
#      differs (or that lacks the feature) is marked MISMATCH for that workload
#      and excluded from its geomean — we only rank shells that computed the
#      same answer.
#
#  This is meant to run INSIDE the docker/Dockerfile.agnostic image, which has
#  all the shells installed. From the repo:   make agnostic-bench
#  Standalone:   ROUNDS=7 bash tests/agnostic_bench.sh
# ============================================================================
set -u
ROUNDS="${ROUNDS:-5}"
TIMEOUT_S="${TIMEOUT_S:-30}"

HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"
HELLISH="${HELLISH:-$ROOT/build/bin/hellish}"

BOLD="\e[1m"; GREEN="\e[0;32m"; RED="\e[0;31m"; YEL="\e[0;33m"
CYAN="\e[0;36m"; GREY="\e[38;5;244m"; END="\e[0m"

# ---- shell roster ----------------------------------------------------------
# Candidate shells in display order. Each has a short LABEL (for the narrow
# matrix columns) and an invocation handled by invoke() below. We probe each
# at startup and keep only the ones that actually exist & run.
CAND=(hellish bash dash zsh mksh ksh yash busybox fish)
declare -A LABEL=(
	[hellish]=hellish [bash]=bash [dash]=dash [zsh]=zsh [mksh]=mksh
	[ksh]=ksh [yash]=yash [busybox]=bbox-ash [fish]=fish
)

# launcher <shell> : the argv PREFIX (everything before `-c`). Most shells are
# a single word; hellish is an absolute path and busybox needs its `ash` applet.
launcher() {
	case "$1" in
		hellish) printf '%s' "$HELLISH";;
		busybox) printf 'busybox ash';;
		*)       printf '%s' "$1";;
	esac
}

# run_shell <out> <shell> <payload> : run the workload under that shell in its
# own natural mode, capped by TIMEOUT_S, stdout → <out>, stderr discarded.
run_shell() {
	local pre; read -ra pre <<< "$(launcher "$2")"
	timeout "$TIMEOUT_S" "${pre[@]}" -c "$3" >"$1" 2>/dev/null
}

# have <shell> : is it installed AND able to run a trivial command?
have() {
	local pre; read -ra pre <<< "$(launcher "$1")"
	command -v "${pre[0]}" >/dev/null 2>&1 || return 1
	"${pre[@]}" -c ':' >/dev/null 2>&1
}

SHELLS=()
for s in "${CAND[@]}"; do have "$s" && SHELLS+=("$s"); done
[[ ${#SHELLS[@]} -eq 0 ]] && { echo -e "${RED}no shells found to benchmark${END}"; exit 1; }

# Oracle for the equality gate: bash if present, else the first shell found.
REF=bash
printf '%s\n' "${SHELLS[@]}" | grep -qx bash || REF="${SHELLS[0]}"

# ---- workloads -------------------------------------------------------------
# Three parallel arrays. POSIX[] is strict /bin/sh (runs on every POSIX shell —
# no [[ ]], no ${//}, no `local`, no arrays). FISH[] is the equivalent in fish
# syntax, or empty to skip fish for that row.
W_NAME=(); W_POSIX=(); W_FISH=()
add() { W_NAME+=("$1"); W_POSIX+=("$2"); W_FISH+=("$3"); }

add "while-arith 50k" \
	'i=0; while [ $i -lt 50000 ]; do i=$((i+1)); done; echo $i' \
	'set i 0; while test $i -lt 50000; set i (math $i + 1); end; echo $i'
add "until-arith 30k" \
	'i=0; until [ $i -ge 30000 ]; do i=$((i+1)); done; echo $i' \
	'set i 0; while not test $i -ge 30000; set i (math $i + 1); end; echo $i'
add "for-seq 5k" \
	'n=0; for x in $(seq 1 5000); do n=$((n+x)); done; echo $n' \
	'set n 0; for x in (seq 1 5000); set n (math $n + $x); end; echo $n'
add "func-call 5k" \
	'f(){ :; }; i=0; while [ $i -lt 5000 ]; do f a b c; i=$((i+1)); done; echo done' \
	'function f; end; set i 0; while test $i -lt 5000; f a b c; set i (math $i + 1); end; echo done'
add "recursion fib18" \
	'fib(){ if [ "$1" -lt 2 ]; then echo "$1"; else a=$(fib $(($1-1))); b=$(fib $(($1-2))); echo $((a+b)); fi; }; fib 18' \
	''
add "var-concat 30k" \
	'a=x; b=y; i=0; while [ $i -lt 30000 ]; do z="$a$b$i"; i=$((i+1)); done; echo "$z"' \
	'set a x; set b y; set i 0; while test $i -lt 30000; set z "$a$b$i"; set i (math $i + 1); end; echo "$z"'
add "concat-grow 4k" \
	's=; i=0; while [ $i -lt 4000 ]; do s="$s"x; i=$((i+1)); done; echo ${#s}' \
	''
add "cmdsub 3k" \
	'i=0; t=0; while [ $i -lt 3000 ]; do v=$(echo $i); t=$((t+1)); i=$((i+1)); done; echo $t' \
	''
add "param-expand 20k" \
	'p=/a/b/c/file.txt; i=0; while [ $i -lt 20000 ]; do b=${p##*/}; d=${p%/*}; i=$((i+1)); done; echo "$b $d"' \
	'set p /a/b/c/file.txt; for i in (seq 1 20000); set b (string replace -r ".*/" "" $p); set d (string replace -r "/[^/]*\$" "" $p); end; echo "$b $d"'
add "default-expand 20k" \
	'u=; i=0; while [ $i -lt 20000 ]; do x=${u:-fallback}; y=${u:+alt}; i=$((i+1)); done; echo "$x-$y"' \
	''
add "strlen-expand 20k" \
	's=abcdefghijklmnop; i=0; n=0; while [ $i -lt 20000 ]; do n=$((n+${#s})); i=$((i+1)); done; echo $n' \
	'set s abcdefghijklmnop; set n 0; for i in (seq 1 20000); set n (math $n + (string length $s)); end; echo $n'
add "case-loop 20k" \
	'i=0; while [ $i -lt 20000 ]; do case $i in *0) :;; *[13579]) :;; *) :;; esac; i=$((i+1)); done; echo $i' \
	'set i 0; while test $i -lt 20000; switch $i; case "*0"; case "*"; end; set i (math $i + 1); end; echo $i'
add "test-string 20k" \
	'a=hello; b=world; i=0; n=0; while [ $i -lt 20000 ]; do if [ "$a" != "$b" ]; then n=$((n+1)); fi; i=$((i+1)); done; echo $n' \
	'set a hello; set b world; set n 0; for i in (seq 1 20000); if test "$a" != "$b"; set n (math $n + 1); end; end; echo $n'
add "ifs-split 10k" \
	'v="a b c d e f g h"; i=0; while [ $i -lt 10000 ]; do set -- $v; i=$((i+1)); done; echo $#' \
	''
add "echo-args 10k" \
	'i=0; while [ $i -lt 10000 ]; do echo a b c d e; i=$((i+1)); done' \
	'for i in (seq 1 10000); echo a b c d e; end'
add "printf-loop 10k" \
	'i=0; while [ $i -lt 10000 ]; do printf "%s-%d\n" abc $i; i=$((i+1)); done' \
	'for i in (seq 0 9999); printf "%s-%d\n" abc $i; end'
add "arith-mix 20k" \
	'i=0; x=0; while [ $i -lt 20000 ]; do x=$(( (i*3+7)%11 )); i=$((i+1)); done; echo $x' \
	'set i 0; set x 0; while test $i -lt 20000; set x (math "($i*3+7)%11"); set i (math $i + 1); end; echo $x'

NW=${#W_NAME[@]}

# ---- timing ----------------------------------------------------------------
# best_us <out> <shell> <payload> : run ROUNDS times, return MIN microseconds.
# min = the least OS-contended run; µs avoids ms quantization on ~1ms scripts.
best_us() {
	local best=999999999 r s e us
	for ((r=0; r<ROUNDS; r++)); do
		s=$(date +%s%N)
		run_shell "$1" "$2" "$3"
		e=$(date +%s%N)
		us=$(( (e - s) / 1000 ))
		(( us < best )) && best=$us
	done
	echo "$best"
}

# ---- accumulators ----------------------------------------------------------
declare -A LOGSUM N WALL          # keyed by shell name, geomean inputs
ms() { awk -v u="$1" 'BEGIN{printf "%.1f", u/1000}'; }

# ---- artifact --------------------------------------------------------------
# The matrix above is for humans; this TSV is for the chart generator
# (bench/lib/collect_data.py -> bench/lib/gen_charts.py).  Written even when
# running inside docker, where the caller bind-mounts or cats it out.
# Sentinel timings: -1 = output mismatched the oracle, -2 = no translation
# for this shell (fish only).  Both mean "excluded from the geomean".
AGN_TSV="${AGN_TSV:-$HERE/agnostic_results.tsv}"
{
	echo "# hellish agnostic bench  $(uname -srm)"
	echo "# rounds=$ROUNDS oracle=$REF shells=${SHELLS[*]}"
	echo "# cell: cell<TAB>workload<TAB>shell<TAB>us"
	echo "# rank: rank<TAB>place<TAB>shell<TAB>n<TAB>geomean_us<TAB>wall_us"
} > "$AGN_TSV" 2>/dev/null || AGN_TSV=/dev/null

# ---- header ----------------------------------------------------------------
NAMW=20; COLW=9
echo -e "${BOLD}cross-shell speed matrix${END}  ${GREY}(cells = best-of-$ROUNDS ms; lower is faster; oracle=$REF)${END}"
echo -e "${GREY}shells: ${SHELLS[*]}${END}\n"
printf "${BOLD}%-${NAMW}s${END}" "workload"
for s in "${SHELLS[@]}"; do printf "%${COLW}s" "${LABEL[$s]}"; done
echo

# ---- run -------------------------------------------------------------------
refout=$(mktemp); shout=$(mktemp)
for ((w=0; w<NW; w++)); do
	name="${W_NAME[$w]}"; pl="${W_POSIX[$w]}"; fpl="${W_FISH[$w]}"
	# oracle output (single untimed run under REF)
	run_shell "$refout" "$REF" "$pl"

	# collect this row, remembering the min for highlighting
	declare -a cell_us=(); declare -a cell_txt=()
	rmin=999999999
	for si in "${!SHELLS[@]}"; do
		s="${SHELLS[$si]}"
		if [[ "$s" == fish ]]; then
			[[ -z "$fpl" ]] && { cell_us[$si]=-2; cell_txt[$si]="·"
				printf 'cell\t%s\t%s\t-2\n' "${name// /_}" "$s" >> "$AGN_TSV"
				continue; }
			use="$fpl"
		else
			use="$pl"
		fi
		us=$(best_us "$shout" "$s" "$use")
		if ! diff -q "$shout" "$refout" >/dev/null 2>&1; then
			cell_us[$si]=-1; cell_txt[$si]="--"
			printf 'cell\t%s\t%s\t-1\n' "${name// /_}" "$s" >> "$AGN_TSV"
			continue
		fi
		(( us < 1 )) && us=1
		cell_us[$si]=$us; cell_txt[$si]="$(ms "$us")"
		printf 'cell\t%s\t%s\t%s\n' "${name// /_}" "$s" "$us" >> "$AGN_TSV"
		(( us < rmin )) && rmin=$us
		LOGSUM[$s]=$(awk -v a="${LOGSUM[$s]:-0}" -v u="$us" 'BEGIN{printf "%.6f", a+log(u)}')
		N[$s]=$(( ${N[$s]:-0} + 1 ))
		WALL[$s]=$(( ${WALL[$s]:-0} + us ))
	done

	printf "%-${NAMW}s" "$name"
	for si in "${!SHELLS[@]}"; do
		if [[ "${cell_us[$si]}" -ge 0 && "${cell_us[$si]}" -eq "$rmin" ]]; then
			printf "${GREEN}${BOLD}%${COLW}s${END}" "${cell_txt[$si]}"
		elif [[ "${cell_us[$si]}" == "-1" ]]; then
			printf "${YEL}%${COLW}s${END}" "${cell_txt[$si]}"
		elif [[ "${cell_us[$si]}" == "-2" ]]; then
			printf "${GREY}%${COLW}s${END}" "${cell_txt[$si]}"
		else
			printf "%${COLW}s" "${cell_txt[$si]}"
		fi
	done
	echo
done
rm -f "$refout" "$shout"

# ---- ranking ---------------------------------------------------------------
echo
echo -e "${BOLD}════════════════════════ RANKING ════════════════════════${END}"
echo -e "${GREY}geomean of per-workload time across matching workloads; ratio vs hellish (>1 ⇒ hellish faster)${END}\n"

hgeo=$(awk -v s="${LOGSUM[hellish]:-0}" -v n="${N[hellish]:-0}" \
	'BEGIN{print (n>0)? exp(s/n) : 0}')

# emit "geomean_us shell n wall_us" then sort ascending by geomean (fastest first)
rank=$(for s in "${SHELLS[@]}"; do
	n=${N[$s]:-0}; [[ $n -eq 0 ]] && continue
	g=$(awk -v l="${LOGSUM[$s]:-0}" -v n="$n" 'BEGIN{printf "%.3f", exp(l/n)}')
	echo "$g $s $n ${WALL[$s]:-0}"
done | sort -n)

printf "  ${BOLD}%-4s %-10s %4s %12s %12s %10s${END}\n" "#" "shell" "n" "geomean" "wall-total" "vs hellish"
i=0
while read -r g s n wall; do
	i=$((i+1))
	printf 'rank\t%s\t%s\t%s\t%s\t%s\n' "$i" "$s" "$n" "$g" "$wall" >> "$AGN_TSV"
	ratio=$(awk -v g="$g" -v h="$hgeo" 'BEGIN{print (h>0)? sprintf("%.2fx", g/h) : "-"}')
	tag=""; col="$END"
	[[ "$s" == hellish ]] && { tag="  ◀ hellish"; col="$CYAN"; }
	printf "${col}  %-4s %-10s %4s %10sms %10sms %10s${END}%s\n" \
		"$i" "$s" "$n" "$(ms "$g")" "$(ms "$wall")" "$ratio" "$tag"
done <<< "$rank"

# ---- verdict ---------------------------------------------------------------
echo
hrank=$(echo "$rank" | grep -n ' hellish ' | cut -d: -f1)
total=$(echo "$rank" | grep -c .)
faster_than=$(echo "$rank" | awk -v h="$hgeo" '$2!="hellish" && $1+0 > h+0 {printf "%s ", $2}')
slower_than=$(echo "$rank" | awk -v h="$hgeo" '$2!="hellish" && $1+0 < h+0 {printf "%s ", $2}')
if [[ -n "${hrank:-}" ]]; then
	echo -e "  ${BOLD}hellish ranks #${hrank} of ${total} shells.${END}"
	[[ -n "$faster_than" ]] && echo -e "  ${GREEN}faster than:${END} ${faster_than}"
	[[ -n "$slower_than" ]] && echo -e "  ${RED}slower than:${END} ${slower_than}"
	if [[ -z "$slower_than" ]]; then
		echo -e "  ${GREEN}${BOLD}VERDICT: hellish is the FASTEST shell in this matrix.${END}"
	elif [[ -z "$faster_than" ]]; then
		echo -e "  ${RED}${BOLD}VERDICT: hellish is the slowest shell in this matrix.${END}"
	elif (( hrank * 3 <= total )); then
		echo -e "  ${GREEN}${BOLD}VERDICT: hellish is near the top (#${hrank}/${total}), behind only:${END} ${slower_than}"
	elif (( hrank * 3 <= total * 2 )); then
		echo -e "  ${YEL}${BOLD}VERDICT: hellish sits mid-pack (#${hrank}/${total}).${END}"
	else
		echo -e "  ${RED}${BOLD}VERDICT: hellish is near the bottom (#${hrank}/${total}).${END}"
	fi
fi
