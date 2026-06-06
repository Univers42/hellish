#!/bin/sh
# Tokenize a sentence, lowercase it, sort unique, and number the result. A
# pure-pipeline exercise over tr / sort / nl.
text="The quick Brown fox The lazy Dog the QUICK fox jumps"
printf '%s\n' "$text" | tr ' ' '\n' | tr 'A-Z' 'a-z' | LC_ALL=C sort -u | nl
