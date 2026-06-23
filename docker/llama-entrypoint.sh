#!/bin/sh
# Pick the GGUF model and launch llama-server. Honour $MODEL_FILE if the user
# pinned one; otherwise grab the first *.gguf in /models. No model => tell the
# user how to fetch one and fail loudly (a server with no weights is useless).
set -eu

if [ -n "${MODEL_FILE:-}" ]; then
	MODEL="/models/${MODEL_FILE}"
else
	# ponytail: "first *.gguf wins" -- fine for the common one-model setup.
	# For multiple models, set MODEL_FILE explicitly (or add a picker).
	MODEL="$(find /models -maxdepth 1 -name '*.gguf' 2>/dev/null | head -n 1)"
fi

if [ -z "${MODEL:-}" ] || [ ! -f "$MODEL" ]; then
	echo "hellish-ai: no model found in /models." >&2
	echo "  Download one into the volume first, e.g.:" >&2
	echo "    make ai-pull MODEL=https://huggingface.co/.../model.gguf" >&2
	echo "  Or set MODEL_FILE=<name.gguf> if the file is already there." >&2
	exit 1
fi

echo "hellish-ai: serving $MODEL on 0.0.0.0:8080" >&2
exec llama-server --host 0.0.0.0 --port 8080 --model "$MODEL" "$@"
