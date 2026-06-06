#!/bin/sh
# Here-documents (expanded vs quoted) plus a quoting matrix. The expanded body
# runs command substitution and arithmetic; the quoted body stays literal.
name="World"
cat <<EOF
Hello, $name!
Today is $(echo payday).
Math: $((6 * 7))
EOF
cat <<'EOF'
Literal: $name $(echo nope) $((1 + 1))
EOF
printf '%s\n' "double: $name" 'single: $name' "mixed: "'$name'" end"
