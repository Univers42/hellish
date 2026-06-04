#!/bin/sh
# Here-doc with <<- which strips LEADING TABS from body and delimiter.
# NOTE: hellish currently does NOT strip tabs (real bug). Kept to surface it.
greet() {
	name=$1
	cat <<-EOF
		Dear $name,
		  (a line with two real leading spaces kept after tab-strip)
		Regards.
	EOF
}

greet Alice
echo "--- plain <<- with delimiter not indented ---"
	cat <<-END
		indented body line
END
