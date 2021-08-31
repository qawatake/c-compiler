#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "9cc.h"

// 現在注目しているトークン
Token *token;

// ローカル変数
LVar *locals;

// 入力プログラム
char *user_input;

// 複数の式
Node *code[100];

// エラーを報告するための関数
// printfと同じ引数を取る
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
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
  // for (;;)
  // {
  //   printf("%c %d\n", token->str[0], token->len);
  //   if (token->kind == TK_EOF)
  //   {
  //     break;
  //   }
  //   token = token->next;
  // }
  program();

  // アセンブリの前半部分を出力
  printf(".intel_syntax noprefix\n");
  printf(".globl main\n");
  printf("main:\n");

  // プロローグ
  // 変数26個分の領域を確保する
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  printf("  sub rsp, 208\n");

  // 先頭の式から順にコード生成
  for (int i=0; code[i]; i++)
  {
    gen(code[i]);

    // 式の評価結果としてスタックに値が1つ残っているはずなので, スタックが溢れないようにポップしておく
    printf("  pop rax\n");

  }

  // エピローグ
  // 最後の式の結果が RAX に残っているのでそれが返り値になる
  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");
  return 0;
}