#!/bin/bash

EXT="$1"
SEARCH="$2"
REPLACE="$3"

if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <extension> <search_pattern> <replace_string>"
    exit 1
fi

find . -type f -name "*$EXT" | while read -r file; do