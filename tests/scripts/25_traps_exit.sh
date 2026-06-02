#!/bin/sh
# EXIT trap for cleanup; trap fires once on normal exit. Deterministic output.
work=$(mktemp -d)
cleanup() {
	echo "cleanup: removing workdir"
	rm -rf "$work"
}
trap cleanup EXIT

echo "body start"
echo "marker" > "$work/marker"
if [ -f "$work/marker" ]; then
	echo "marker exists"
fi
echo "body end"
# Falling off the end triggers the EXIT trap.
