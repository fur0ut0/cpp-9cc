#!/bin/bash
set -eu

test_return_code() {
   program="$1"
   expected="$2"
   input="$3"

   ./"$program" "$input" > tmp.s
   cc -o tmp tmp.s
   ./tmp && true
   actual="$?"

   if [ "$actual" = "$expected" ]; then
      echo "[PASS] $input => $actual"
   else
      echo "[FAIL] $input => expected=$expected, actual=$actual"
      return 1
   fi
}

test_return_code 9cc 0 0
test_return_code 9cc 42 42
test_return_code 9cc 21 "5+20-4"
test_return_code 9cc 41 " 12 + 34 - 5 "

echo "OK"
