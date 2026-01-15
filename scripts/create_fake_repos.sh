#!/usr/bin/env bash
set -e

ROOT="glob-zoo"
rm -rf "$ROOT"
mkdir -p "$ROOT"
cd "$ROOT"

echo "🧪 Creating glob torture repo..."

# ---------- directory depth ----------
mkdir -p shallow
mkdir -p deep/a/b/c
mkdir -p deep/x/y/z
mkdir -p dotfiles
mkdir -p symbols
mkdir -p numeric
mkdir -p mixed

# ---------- basic files ----------
touch README.md
touch test.md test.c test.txt
touch test1 test2 testA testa
touch test_file test-file test.file
touch testfile testXfile testYfile

# ---------- question mark / length ----------
touch test1test test2test testAtest
touch aaaa bbbb cccc
touch testAAA testBBB

# ---------- ranges ----------
touch testa testb testc testd
touch test0 test5 test9
touch testa1 testb2 testc3
touch testz9 testA0

# ---------- bracket madness ----------
touch testa- test-a test[a] test[a-z]
touch test- test--
touch testabc testdef testghi
touch abc def ghi

# ---------- numeric ----------
touch numeric/0 numeric/1 numeric/9
touch numeric/a1 numeric/1a numeric/99
touch numeric/file1 numeric/file2

# ---------- POSIX classes ----------
touch Alpha
touch LOWER
touch upper lower
touch MixedCase
touch space\ file
touch tab$'\t'file 2>/dev/null || true

# ---------- symbols ----------
touch symbols/'*'
touch symbols/'?'
touch symbols/'['
touch symbols/']'
touch symbols/'!'
touch symbols/'^'
touch symbols/'-'
touch symbols/'\*'
touch symbols/'\?'

# ---------- dotfiles ----------
touch .hidden
touch .test
touch .testfile
touch dotfiles/.a
touch dotfiles/.z

# ---------- multi-extension ----------
touch test.md.bak
touch test.backup.md
touch test.xx.c
touch test.aa.c

# ---------- deep files ----------
touch deep/a/b/c/test1
touch deep/a/b/c/testA
touch deep/a/b/c/file9
touch deep/x/y/z/alpha1
touch deep/x/y/z/beta2

# ---------- wild concatenations ----------
touch a*b a*b*c
touch aXbXc
touch test123test
touch testABCtest

# ---------- underscore / word ----------
touch test_word
touch test123_
touch _private
touch __init__

echo "✅ Repo created: $ROOT"
echo "👉 cd into it and run your glob echo tests"
