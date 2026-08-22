#!/bin/bash
cd /home/dylan/sh42
touch src/parsing/compound_list.c
make all 2>&1 | tee /tmp/make_out.txt
echo "BUILD_EXIT:$?"
