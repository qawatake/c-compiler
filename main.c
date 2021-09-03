#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "9cc.h"

// 現在注目しているトークン
Token *token;

// ローカル変数
LVar *locals;

// 入力プログラム
char *user_input;

// ループの数
int lnum = 0;

// 複数の関数
Function *fns[100];

// エラーを報告するための関数
// printfと同じ引数を取る
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

char* duplicate(char *str, size_t len) {

    char *buffer = malloc(len + 1);
    memcpy(buffer, str, len);
    buffer[len] = '\0';

    return buffer;
}

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    error("引数の個数が正しくありません");
    return 1;
  }

  // トークナイズしてパースする
  user_input = argv[1];
  token = tokenize(user_input);
  program();

  // アセンブリの前半部分を出力
  printf(".intel_syntax noprefix\n");
  for (int i = 0; fns[i]; i++)
  {
    // gen(code[i]);
    gen_func(fns[i]);
  }

  return 0;
}