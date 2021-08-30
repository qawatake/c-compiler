#!/bin/bash
assert() {
  expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s
  cc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 0 0
assert 42 42
assert 21 "5+20-4"
assert 41 " 12 + 34 - 5 "
assert 47 '5+6*7'
assert 15 '5*(9-6)'
assert 4 '(3+5)/2'
assert 0 '-5+5'
assert 10 '-10+20'
assert 0 '-(3+5) + 8'
assert 1 '-3*+5 + 16'
assert 1 '11==11'
assert 0 '12!=12'
assert 1 '1<=2>=2==0'
assert 1 '(12 + 2 + 1) == 3 * 5 *1'

echo OK