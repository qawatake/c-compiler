#!/bin/bash
assert() {
  expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s
  cc -o tmp tmp.s tmp-test.o
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

cmp() {
  input1="$1"
  input2="$2"

  ./9cc "$input1" > tmp1.s
  ./9cc "$input2" > tmp2.s

  diff tmp1.s tmp2.s > /dev/null
  if [ $? = 0 ]; then
    echo "same assembly from $input1 and $input2"
  else
    echo "different assembly from $input1 and $input2"
    exit 1
  fi
}

cat <<EOF | gcc -xc -c -o tmp-test.o -
#include <stdlib.h>
int plus(int x, int y) { return x + y; }
void alloc4(int **q, int a, int b, int c, int d)
{
  *q = malloc(4 * sizeof(int));
  (*q)[0] = a; (*q)[1] = b;
  (*q)[2] = c; (*q)[3] = d;
}
void palloc2(int ***q, int a, int b)
{
  *q = malloc(2 * sizeof(int **));
  int **p = *q;
  p[0] = malloc(sizeof(int*));
  p[1] = malloc(sizeof(int*));
  *(p[0]) = a;
  *(p[1]) = b;
}
EOF

assert 0 "int main(){struct {} x; return sizeof x;}"
assert 4 "int main(){struct {int a;} x; return sizeof x;}"
assert 8 "int main(){struct{char a; char b; int c;} x; return sizeof x;}"
assert 12 "int main(){struct{char a; char b; int c; char d;} x; return sizeof x;}"
assert 3 "int main(){struct{int a;} x; x.a = 3; return x.a;}"
assert 8 "int main(){struct{char a; int b;} x; x.a = 3; x.b = 5; return x.a + x.b;}"
assert 11 "int main(){struct{struct{char a;} b; int c;} x; x.b.a = 1; x.c = 2; return x.b.a + x.c + sizeof x;}"
assert 11 "struct{struct{char a;} b; int c;} x; int main(){x.b.a = 1; x.c = 2; return x.b.a + x.c + sizeof x;}"
assert 8 "int main(){struct{char a[3]; int b;} x; x.a[0] = 1; x.a[2] = 3; x.b = 4; return x.a[0] + x.a[2] + x.b;}"
assert 8 "int main(){struct { char a; int b; } x; struct { char a; int b; } *p = &x; x.a=3; x.b=5; return p->a+p->b;}"
assert 8 "int main(){struct tag { char a; int b; } x; struct tag *p = &x; x.a=3; x.b=5; return p->a+p->b;}"
assert 3 "int main(){typedef int foo; foo x = 3; return x;}"
assert 8 "int main(){struct tag { char a; int b; } x; typedef struct tag tag; tag *p = &x; x.a=3; x.b=5; return p->a+p->b;}"

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
assert 3 "int main(){int a[2]; *a = 1; *(a + 1) = 2; int *p; p = a; return *p + *(p + 1);}"
assert 2 "int main(){int x[2]; x[0] = 1; x[1] = 2; return 1[x];}"
assert 2 'int main(){int x[2]; x[0] = 1; x[1] = 2; return x[1];}'
assert 2 "int main(){int *y; int x; y = &x; x = 2; y[0];}"
assert 1 "int main(){int a; a = 1; {int a; a = 100;} return a;}"
assert 2 "int main(){int a; a = 1; {int b; b = 1; a = a + b;} return a;}"
assert 13 "int main(){int a[3]; a[0] = 13; return **&a;}"
assert 24 "int main(){int a[2][3]; return sizeof(a);}"
assert 8 "int main(){int (*a)[3]; return sizeof a;}"
assert 4 "int main(){int x[3]; int (*y)[3]; y = &x; x[0] = 4; return **y;}"
assert 24 "int main(){int *(*x[3])[4]; return sizeof x;}"
assert 1 "int a; int main(){a = 1; return a;}"
assert 3 "int *x; int main(){int a; x = &a; a = 3; return *x;}"
assert 3 "int a[4]; int main(){a[0] = 1; a[2] = 2; return a[0] + a[2];}"
assert 3 "int main(){char x[3]; x[0] = -1; x[1] = 2; int y; y = 4; return x[0] + y;}"
assert 3 "int main(){char x[3]; return sizeof x;}"
assert 97 'int main() { char *p = "abc"; return p[0]; }'
assert 98 'int main() { char *p = "abc"; return p[1]; }'
assert 99 'int main() { char *p = "abc"; return p[2]; }'
assert 0 'int main() { char *p = "abc"; return p[3]; }'
assert 37 'int main(){char *format; format = "%d %s <= printed by ↓\n"; printf(format, 10, "hello, world"); return format[0];}'
assert 1 "int main(){int x = 1; return x;}"
assert 99 'int main(){char *a = "abc"; return a[2];}'
assert 21 "int a = 10; char b = 11; int main(){return a + b;}"
assert 108 'char *p = "hello, world"; int main(){return p[2];}'
assert 2 'int a = 1; int *x = &a; int main(){a = 2; return *x;} '
assert 1 "int main(){int a = 1; /* a = 2; */ return a;}"
assert 6 "int main(){int a[3]; a[0] = 1; a[1] = 2; a[2] = 3; /* a[1] = 5 */ return a[0] + a[1] + a[2];}"
assert 15 "int main(){int x[3] = {13, 14, 15}; return x[2];}"
assert 1 "int main(){int x[3] = {}; x[1] = 1; return x[1];}"
assert 2 "int main(){int a = 1; int b = 2; int c = 3; int *x[3] = {&a, &b, &c}; return *x[1];}"
assert 0 "int main(){char x[3] = {1, 2}; return x[2];}"
assert 10 "int main(){int x[20] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}; return x[10];}"
assert 0 "int main(){int x[20] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}; return x[11] + x[12] + x[13] + x[14] + x[15] + x[16] + x[17] + x[18] + x[19];}"
assert 2 "int main(){int x[] = {0, 1, 2}; return x[2];}"
assert 11 'int main(){int x = 2; int a[2][2] = {{x, x + 5}, {x * x, x * 3}}; return a[0][1] + a[1][0];}'
assert 101 "int main(){char a[] = {'h', 'e', 'l', 'l', 'o', '\0'}; return a[1];}"
assert 97 "int main(){char a = 'a'; return a;}"
assert 101 'int main(){char x[6] = "hello"; return x[1];}'
assert 101 'int main(){char x[] = "hello"; return x[1];}'
assert 197 'int main(){char *x[] = {"ab", "cd"}; return x[0][1] + x[1][0];}'
assert 197 'int main(){char x[][3] = {"ab", "cd"}; return x[0][1] + x[1][0];}'
assert 37 'int a = 1; int *x = 1 + 4 - 3 + &a - 3; int main(){char *format; format = "%p %p <= printed by ↓\n"; printf(format, &a, x); return format[0];}'
assert 1 'int a = 1; int *x = &a; int main(){return *x;}'
assert 19 "int a = 1 + 2 - 3 + 4 * 4 - 6 / 5 + (3 == 3) + (3 != 3) + (4 < 5) + (4 > 5) + (4 <= 5) + (5 >= 4); int main(){return a;}"
assert 19 "char a = 1 + 2 - 3 + 4 * 4 - 6 / 5 + (3 == 3) + (3 != 3) + (4 < 5) + (4 > 5) + (4 <= 5) + (5 >= 4); int main(){return a;}"
assert 37 'int a = 1; int *x = 1 + 4 - 3 + &a - 3; int main(){char *format; format = "%p %p <= printed by ↓\n"; printf(format, &a, x); return format[0];}'
assert 1 'int a = 1; int *x = &a; int main(){return *x;}'
assert 98 'char *a = "abc"; int main(){return a[1];}'
assert 99 'char a[] = "abc"; int main(){return a[2];}'
assert 33 "int x[3] = {1, 2 + 3, 4 + 5 + 6}; int main(){return sizeof(x) + x[0] + x[1] + x[2];}"
assert 33 "int x[] = {1, 2 + 3, 4 + 5 + 6}; int main(){return sizeof(x) + x[0] + x[1] + x[2];}"
assert 2 "int a; int *x[] = {&a, &a +1}; int main(){a = 2; return *x[0];}"
assert 7 "int a[3]; int *x[3] = {a, a+1, a+2}; int main(){a[0] = 1; a[1] = 2; a[2] = 4; return *x[0] + *x[1] + *x[2];}"

# 出力されるアセンブリの比較
cmp 'int main(){char x[6] = "hello"; return x[1];}' 'int main(){char x[] = "hello"; return x[1];}'
cmp "int main(){int x[3] = {0, 1, 2}; return x[2];}" "int main(){int x[] = {0, 1, 2}; return x[2];}"
cmp "int main(){char a[6] = {'h', 'e', 'l', 'l', 'o', '\0'}; return a[1];}" "int main(){char a[] = {'h', 'e', 'l', 'l', 'o', '\0'}; return a[1];}"
cmp 'int main(){int x = 2; int a[][2] = {{x, x + 5}, {x * x, x * 3}}; return a[0][1] + a[1][0];}' 'int main(){int x = 2; int a[2][2] = {{x, x + 5}, {x * x, x * 3}}; return a[0][1] + a[1][0];}'

echo OK