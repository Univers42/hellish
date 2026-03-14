echo "=== FUNCTION DEMO ==="

echo ""
echo "--- 1. Basic function ---"
greet() { echo "Hello, World!"; }
greet

echo ""
echo "--- 2. Function with arguments ---"
say_hello() { echo "Hello, $1!"; }
say_hello Dylan
say_hello World

echo ""
echo "--- 3. Arithmetic in functions ---"
add() { echo $(( $1 + $2 )); }
multiply() { echo $(( $1 * $2 )); }
add 3 7
multiply 6 8

echo ""
echo "--- 4. Functions with control flow ---"
is_even() {
  if [ $(( $1 % 2 )) -eq 0 ]; then
    echo "$1 is even"
  else
    echo "$1 is odd"
  fi
}
is_even 4
is_even 7
is_even 42

echo ""
echo "--- 5. Functions with loops ---"
countdown() {
  for i in 5 4 3 2 1; do
    echo -n "$i "
  done
  echo "Go!"
}
countdown

echo ""
echo "--- 6. Function calling function ---"
double() { echo $(( $1 * 2 )); }
quadruple() { double $(( $1 * 2 )); }
double 5
quadruple 3

echo ""
echo "--- 7. Function overwrite ---"
msg() { echo "version 1"; }
msg
msg() { echo "version 2"; }
msg

echo ""
echo "--- 8. Multiple arguments ---"
greet_full() { echo "Hello $1 $2, welcome to $3!"; }
greet_full Dylan Lesieur Hellish

echo ""
echo "--- 9. Function with pipe ---"
upper_first() { echo "$1" | tr a-z A-Z; }
upper_first hello

echo ""
echo "--- 10. Positional params count ---"
show_count() { echo "Got $# arguments"; }
show_count a b c d e

echo ""
echo "=== ALL FUNCTION DEMOS COMPLETE ==="
