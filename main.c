#include <stdio.h>
#include "9cc.h"

// 現在注目しているトークン
Token *token;

// 入力プログラム
char *user_input;

// 複数の式
Node *code[100];

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    fprintf(stderr, "引数の個数が正しくありません\n");
    return 1;
  }

  // トークナイズしてパースする
  user_input = argv[1];
  token = tokenize(user_input);
  Node *node = expr();

  // アセンブリの前半部分を出力
  printf(".intel_syntax noprefix\n");
  printf(".globl main\n");
  printf("main:\n");

  // 抽象構文木を降りながらコード生成
  gen(node);

  printf("  pop rax\n");
  printf("  ret\n");
  return 0;
}