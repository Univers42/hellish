#!/bin/sh
# Arithmetic with different bases, bitwise ops, ternary, comparisons.
echo "hex:   $((0xFF)) $((0x10))"
echo "octal: $((010)) $((0777))"
echo "base2: $((2#1010)) $((2#11111111))"
echo "base16ex: $((16#abc))"

echo "bitand: $((12 & 10))"
echo "bitor:  $((12 | 3))"
echo "bitxor: $((12 ^ 10))"
echo "shl:    $((1 << 8))"
echo "shr:    $((1024 >> 3))"

echo "ternary: $(( 5 > 3 ? 100 : 200 ))"
echo "nested:  $(( 1 ? (2 ? 30 : 40) : 50 ))"

echo "cmp: $((3 == 3)) $((3 != 4)) $((3 <= 3)) $((4 >= 5))"
echo "logic: $((1 && 0)) $((1 || 0)) $((!0))"

a=5
a=$((a += 3))
echo "compound-add: $a"
a=$((a *= 2))
echo "compound-mul: $a"

echo "precedence: $((2 + 3 * 4 - 1))"
echo "paren:      $(((2 + 3) * 4))"
