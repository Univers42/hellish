echo === LEVEL 06: If Statements ===
if true; then echo "if-true works"; fi
if false; then echo "FAIL"; fi
echo "after false if: $?"
if false; then echo "FAIL"; else echo "else works"; fi
if false; then echo "FAIL"; elif true; then echo "elif works"; else echo "FAIL"; fi
if false; then echo "FAIL"; elif false; then echo "FAIL"; else echo "final else works"; fi
X=5
if [ $X -eq 5 ]; then echo "X equals 5"; fi
if [ $X -gt 10 ]; then echo "FAIL"; else echo "X not greater than 10"; fi
