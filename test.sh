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

assert 0 "0;"
assert 42 "42;"
assert 21 "5+20-4;"
assert 41 " 12 + 34 - 5 ;"
assert 47 '5+6*7;'
assert 15 '5*(9-6);'
assert 4 '(3+5)/2;'
assert 0 '-5+5;'
assert 10 '-10+20;'
assert 0 '-(3+5) + 8;'
assert 1 '-3*+5 + 16;'
assert 1 '11==11;'
assert 0 '12!=12;'
assert 1 '1<=2>=2==0;'
assert 1 '(12 + 2 + 1) == 3 * 5 *1;'
assert 6 'a=3; a = a *2; a;'
assert 12 'a=3; b=2; c= a * b; c *2;'
assert 6 'abc = 4; d = 2; abc + d;'
assert 6 'ret = 4; urn =2; return ret + urn;'
assert 3 'return 3; return 4;'
assert 2 'if (0 == 0) return 2;'
assert 13 'a = 4; if (a / 2 == 2) return 13; return 20;'
assert 8 'a = 5; if (a / 2 == 1) a = a * 2; if (a / 2 == 2) a = a + 3; return a;'
assert 1 'if (0!=0) return 0; else return 1;'
assert 8 'a = 5; if (a / 2 == 1) a = a * 2; else a = a + 3; return a;'
assert 3 'i = 0; while (i < 3) i = i + 1; return i;'
assert 16 'i=1; while (i<10) i = i * 2; return i;'
assert 3 'for (i=0; i<=2; i = i+1) i; return i;'
assert 3 'acc = 0; for (i=0; i <= 2; i = i + 1) acc = acc + i; return acc;'

echo OK