#!/bin/bash
set -eu

test_return_code() {
   program="$1"
   exptected="$2"
   input="$3"

   "$program" "$input" > tmp.s
   cc -o tmp tmp.s
   ./tmp
   actual="$?"

   if [ "$actual" = "$expected" ]; then
      echo "[PASS] $input => $actual"
   else
      echo "[FAIL] $input => expected=$expected, actual=$actual"
      return 1
   fi
}

test_return_code "$1" 0 0
test_return_code "$1" 42 42

echo "OK"
