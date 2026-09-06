# BASH_SOURCE when the script is RUN AS A FILE -- issue #118.
#
# A sourced file could already name itself; a script run as `hellish x.sh`
# could not: ${BASH_SOURCE[0]} was unset, so `dirname "${BASH_SOURCE[0]}"`
# resolved to `.` and the run-vs-source guard `[ "${BASH_SOURCE[0]}" = "$0" ]`
# was false, skipping every action block.  Every line here must match
# bash --posix byte for byte; paths are printed as basenames so the scratch
# directory the corpus runs from never appears in the output.
echo "same-as-0=$([ "${BASH_SOURCE[0]}" = "$0" ] && echo yes || echo no)"
echo "n=${#BASH_SOURCE[@]}"
echo "base=${BASH_SOURCE[0]##*/}"
echo "unset-form=[${BASH_SOURCE[0]:-UNSET}]"
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "here-is-dir=$([ -d "$here" ] && echo yes || echo no)"
echo "here-has-me=$([ -f "$here/${BASH_SOURCE[0]##*/}" ] && echo yes || echo no)"
if [ "${BASH_SOURCE[0]}" = "${0}" ]; then
	echo "guard: run"
else
	echo "guard: sourced"
fi
whereami() {
	echo "fn-base=${BASH_SOURCE[0]##*/} fn=${FUNCNAME[0]} fn-n=${#BASH_SOURCE[@]}"
}
whereami
printf 'echo "in-helper=${BASH_SOURCE[0]##*/} n=${#BASH_SOURCE[@]}"\n' > helper.sh
printf 'helper_fn() { echo "helper-fn-src=${BASH_SOURCE[0]##*/}"; }\n' >> helper.sh
. ./helper.sh
helper_fn
echo "after-source=${BASH_SOURCE[0]##*/} n=${#BASH_SOURCE[@]}"
rm -f helper.sh
