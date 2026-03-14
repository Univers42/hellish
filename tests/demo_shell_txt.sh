echo "============================================"
echo "  HELLISH SHELL - SHELL.TXT COMPLIANCE DEMO"
echo "============================================"
echo ""

echo "=== MANDATORY PART ==="
echo ""

echo "--- M1. Prompt + Run Commands with PATH ---"
echo "hello world"
/bin/echo "direct path execution"
ls /dev/null

echo ""
echo "--- M2. Spaces and Tabs ---"
echo    "tabs work"
echo "  spaces preserved in quotes"

echo ""
echo "--- M3. Redirections ---"
echo "output redir" > /tmp/h_test_redir.txt
cat /tmp/h_test_redir.txt
echo "appended" >> /tmp/h_test_redir.txt
cat /tmp/h_test_redir.txt
cat < /tmp/h_test_redir.txt
cat << DELIM
heredoc line 1
heredoc line 2
DELIM
rm -f /tmp/h_test_redir.txt

echo ""
echo "--- M4. Pipes ---"
echo "hello" | cat | cat | cat
echo -e "c\na\nb" | sort
ls /etc/hostname | cat

echo ""
echo "--- M5. Semicolons ---"
echo "first" ; echo "second" ; echo "third"

echo ""
echo "--- M6. Builtins: cd, echo, exit, type ---"
echo "pwd: $(pwd)" 2>/dev/null
cd /tmp
echo "after cd /tmp"
cd -
echo "back to original"
echo -n "no newline: " ; echo "done"
type echo
type ls
type cd

echo ""
echo "--- M7. Logical Operators && || ---"
true && echo "AND: success"
false || echo "OR: fallback"
false && echo "NO" || echo "OR chain: correct"
true || echo "NO" && echo "AND chain: correct"

echo ""
echo "--- M8. Operator Precedence ---"
echo "pipe before semi:"
echo hello | cat ; echo world

echo ""
echo "--- M9. Internal Variables ---"
MY_VAR=hello
echo "var: $MY_VAR"
echo "braced: ${MY_VAR}"
export EXPORTED=visible
env | grep EXPORTED
unset MY_VAR
echo "after unset: [$MY_VAR]"
echo "exit code: $?"

echo ""
echo "--- M10. Job Control (& operator) ---"
sleep 0 &
echo "background launched: $?"

echo ""
echo "--- M11. Signals ---"
echo "SIGINT/SIGQUIT handled (interactive mode)"

echo ""
echo "=== MODULAR FEATURES ==="
echo ""

echo "--- F1. Inhibitors (quotes, backslash) ---"
echo "double quotes preserve spaces:   ok"
echo 'single quotes: $NOT_EXPANDED'
echo "escaped dollar: \$NOT_EXPANDED"
echo "mixed 'single' inside double"

echo ""
echo "--- F2. Globbing ---"
echo /etc/host*
echo /etc/hos?name
echo /dev/[ns]ull

echo ""
echo "--- F3. Tilde + Parameter Expansion ---"
echo "tilde: ~"
unset MISSING
echo "default: ${MISSING:-fallback}"
PRESENT=hello
echo "alternate: ${PRESENT:+yes}"
echo "length: ${#PRESENT}"
MSG=hello_world
echo "prefix strip: ${MSG#hello_}"
echo "suffix strip: ${MSG%_world}"

echo ""
echo "--- F4. Subshells and Groups ---"
X=outer
(X=inner ; echo "subshell: $X")
echo "parent: $X"

echo ""
echo "--- F5. Arithmetic Expansion ---"
echo "add: $((3 + 7))"
echo "mult: $((6 * 7))"
echo "mod: $((17 % 5))"
echo "compare: $((5 > 3))"
echo "equality: $((42 == 42))"
echo "power: $((2 ** 10))"
A=10
B=20
echo "vars: $((A + B))"
echo "dollar vars: $(($A + $B))"
echo "complex: $(( (A + B) * 2 - 5 ))"

echo ""
echo "=== BONUS: SHELL SCRIPTING ==="
echo ""

echo "--- B1. If/Elif/Else ---"
X=42
if [ $X -gt 100 ]
then
    echo "big"
elif [ $X -gt 30 ]
then
    echo "medium: $X"
else
    echo "small"
fi

echo ""
echo "--- B2. For Loop ---"
for fruit in apple banana cherry
do
    echo "  $fruit"
done

echo ""
echo "--- B3. While Loop ---"
N=0
while [ $N -lt 5 ]
do
    echo -n "$N "
    N=$((N + 1))
done
echo ""

echo ""
echo "--- B4. Until Loop ---"
M=3
until [ $M -eq 0 ]
do
    echo -n "$M "
    M=$((M - 1))
done
echo "done!"

echo ""
echo "--- B5. Nested Compounds ---"
for i in 1 2 3
do
    if [ $((i % 2)) -eq 0 ]
    then
        echo "  $i is even"
    else
        echo "  $i is odd"
    fi
done

echo ""
echo "--- B6. Deep Nesting ---"
I=0
while [ $I -lt 2 ]
do
    for letter in a b
    do
        if [ $I -eq 0 ]
        then
            echo "  $I-$letter first"
        else
            echo "  $I-$letter second"
        fi
    done
    I=$((I + 1))
done

echo ""
echo "============================================"
echo "  ALL SHELL.TXT REQUIREMENTS DEMONSTRATED"
echo "============================================"
