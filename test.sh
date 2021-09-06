#!/bin/bash
assert() {
  expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s
  cc -o tmp tmp.s tmp-plus.o
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

echo 'int plus(int x, int y) { return x + y; }' | cc -xc -c -o tmp-plus.o -

assert 0 "main(){0;}"
assert 42 "main(){42;}"
assert 21 "main(){5+20-4;}"
assert 41 "main(){ 12 + 34 - 5 ;}"
assert 47 'main(){5+6*7;}'
assert 15 'main(){5*(9-6);}'
assert 4 'main(){(3+5)/2;}'
assert 0 'main(){-5+5;}'
assert 10 'main(){-10+20;}'
assert 0 'main(){-(3+5) + 8;}'
assert 1 'main(){-3*+5 + 16;}'
assert 1 'main(){11==11;}'
assert 0 'main(){12!=12;}'
assert 1 'main(){1<=2>=2==0;}'
assert 1 'main(){(12 + 2 + 1) == 3 * 5 *1;}'
assert 6 'main(){int a; a=3; a = a *2; a;}'
assert 12 'main(){int a; int b; int c; a=3; b=2; c= a * b; c *2;}'
assert 6 'main(){int abc; int d; abc = 4; d = 2; abc + d;}'
assert 6 'main(){int ret; int urn; ret = 4; urn =2; return ret + urn;}'
assert 3 'main(){return 3; return 4;}'
assert 2 'main(){if (0 == 0) return 2;}'
assert 13 'main(){int a; a = 4; if (a / 2 == 2) return 13; return 20;}'
assert 8 'main(){int a; a = 5; if (a / 2 == 1) a = a * 2; if (a / 2 == 2) a = a + 3; return a;}'
assert 1 'main(){if (0!=0) return 0; else return 1;}'
assert 8 'main(){int a; a = 5; if (a / 2 == 1) a = a * 2; else a = a + 3; return a;}'
assert 3 'main(){int i; i = 0; while (i < 3) i = i + 1; return i;}'
assert 16 'main(){int i; i=1; while (i<10) i = i * 2; return i;}'
assert 3 'main(){int i; for (i=0; i<=2; i = i+1) i; return i;}'
assert 3 'main(){int acc; int i; acc = 0; for (i=0; i <= 2; i = i + 1) acc = acc + i; return acc;}'
assert 3 'main(){int acc; int i; acc = 0; i = 0; while (i <= 2) {acc = acc + i; i = i + 1;} return acc;}'
assert 5 'main() { return plus(2, 3); }'
assert 1 'one() { return 1; } main() { return one(); }'
assert 3 'one() { return 1; } two() { return 2; } main() { return one()+two(); }'
assert 6 'mul(a, b) { return a * b; } main() { return mul(2, 3); }'
assert 21 'add(a,b,c,d,e,f) { return a+b+c+d+e+f; } main() { return add(1,2,3,4,5,6); }'
assert 6 'mul(a, b) {return a * b;} main(){int a; int b; a = 2; b = 3; return mul(a, b);}'
assert 3 'main(){int x; int y; x = 3; y = &x; return *y;}'
assert 1 'main(){int x; x = 1; return *&x;}'


echo OK