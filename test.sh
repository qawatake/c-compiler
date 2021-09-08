#!/bin/bash
assert() {
  expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s
  cc -o tmp tmp.s tmp-plus.o tmp-alloc4.o tmp-palloc2.o
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
echo "#include <stdlib.h>
void alloc4(int **q, int a, int b, int c, int d) { *q = malloc(4 * sizeof(int)); (*q)[0] = a; (*q)[1] = b; (*q)[2] = c; (*q)[3] = d;}" | cc -xc -c -o tmp-alloc4.o -
echo "#include <stdlib.h>
void palloc2(int ***q, int a, int b) {*q = malloc(2 * sizeof(int **)); int **p = *q; p[0] = malloc(sizeof(int*)); p[1] = malloc(sizeof(int*)); *(p[0]) = a; *(p[1]) = b;}" | cc -xc -c -o tmp-palloc2.o -

assert 0 "int main(){0;}"
assert 42 "int main(){42;}"
assert 21 "int main(){5+20-4;}"
assert 41 "int main(){ 12 + 34 - 5 ;}"
assert 47 'int main(){5+6*7;}'
assert 15 'int main(){5*(9-6);}'
assert 4 'int main(){(3+5)/2;}'
assert 0 'int main(){-5+5;}'
assert 10 'int main(){-10+20;}'
assert 0 'int main(){-(3+5) + 8;}'
assert 1 'int main(){-3*+5 + 16;}'
assert 1 'int main(){11==11;}'
assert 0 'int main(){12!=12;}'
assert 1 'int main(){1<=2>=2==0;}'
assert 1 'int main(){(12 + 2 + 1) == 3 * 5 *1;}'
assert 6 'int main(){int a; a=3; a = a *2; a;}'
assert 12 'int main(){int a; int b; int c; a=3; b=2; c= a * b; c *2;}'
assert 6 'int main(){int abc; int d; abc = 4; d = 2; abc + d;}'
assert 6 'int main(){int ret; int urn; ret = 4; urn =2; return ret + urn;}'
assert 3 'int main(){return 3; return 4;}'
assert 2 'int main(){if (0 == 0) return 2;}'
assert 13 'int main(){int a; a = 4; if (a / 2 == 2) return 13; return 20;}'
assert 8 'int main(){int a; a = 5; if (a / 2 == 1) a = a * 2; if (a / 2 == 2) a = a + 3; return a;}'
assert 1 'int main(){if (0!=0) return 0; else return 1;}'
assert 8 'int main(){int a; a = 5; if (a / 2 == 1) a = a * 2; else a = a + 3; return a;}'
assert 3 'int main(){int i; i = 0; while (i < 3) i = i + 1; return i;}'
assert 16 'int main(){int i; i=1; while (i<10) i = i * 2; return i;}'
assert 3 'int main(){int i; for (i=0; i<=2; i = i+1) i; return i;}'
assert 3 'int main(){int acc; int i; acc = 0; for (i=0; i <= 2; i = i + 1) acc = acc + i; return acc;}'
assert 3 'int main(){int acc; int i; acc = 0; i = 0; while (i <= 2) {acc = acc + i; i = i + 1;} return acc;}'
assert 5 'int main() { return plus(2, 3); }'
assert 1 'int one() { return 1; } int main() { return one(); }'
assert 3 'int one() { return 1; } int two() { return 2; } int main() { return one()+two(); }'
assert 6 'int mul(int a, int b) { return a * b; } int main() { return mul(2, 3); }'
assert 21 'int add(int a, int b, int c, int d, int e, int f) { return a+b+c+d+e+f; } int main() { return add(1,2,3,4,5,6); }'
assert 6 'int mul(int a, int b) {return a * b;} int main(){int a; int b; a = 2; b = 3; return mul(a, b);}'
assert 3 'int main(){int x; int *y; x = 3; y = &x; return *y;}'
assert 1 'int main(){int x; x = 1; return *&x;}'
assert 3 'int main(){int x; int *y; y = &x; *y = 3; return x;}'
assert 3 'int main(){int x; int *y; int **z; y = &x; z = &y; **z = 3; return x;}'
assert 4 'int add1(int *x){ *x = *x+1;} int main(){int x; x = 3; add1(&x); return x;}'
assert 3 'int *pointer(int *z){return z;} int main(){int x; int *y; y = &x; x = 3; return *pointer(y);}'
assert 8 "int main(){int *p; alloc4(&p, 1, 2, 4, 8); int *q; q = 3 + p; return *q;}"
assert 8 "int main(){int *p; alloc4(&p, 1, 2, 4, 8); int *q; q = p + 3; return *q;}"
assert 4 "int main(){int *p; alloc4(&p, 1, 2, 4, 8); int *q; q = p + 3; q = q - 1; return *q;}"
assert 8 'int main(){int **q; palloc2(&q, 4, 8); q = q + 1; return **q;}'
assert 4 "int main(){return sizeof(1);}"
assert 8 "int main(){int *y; return sizeof(y + 3);}"
assert 4 "int main(){int x; return sizeof(x + 3);}"
assert 12 "int main(){int a[3]; return sizeof a;}"
assert 3 "int main(){int a[2]; *a = 3; return *a;}"
# assert 3 "int main(){int a[2]; *a = 1; *(a + 1) = 2; int *p; p = a; return *p + *(p + 1);}"

echo OK