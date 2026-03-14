echo === LEVEL 02: Variables and Environment ===
MY_VAR=hello
echo $MY_VAR
GREETING="hello world"
echo $GREETING
echo "interpolated: $MY_VAR in quotes"
echo 'no interpolation: $MY_VAR in single quotes'
X=42
echo "X is $X"
echo "dollar question: $?"
export EXPORTED_VAR=visible
env | grep EXPORTED_VAR
unset EXPORTED_VAR
echo "after unset: [$EXPORTED_VAR]"
