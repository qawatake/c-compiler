#include <stdio.h>
#include <stdlib.h>
#include "9cc.h"

Token *token;
LVar *locals;
char *user_input;
int lnum = 0;
Scope *scope;
Function *funcs;

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    error("引数の個数が正しくありません");
    return 1;
  }

  // トークナイズ
  user_input = argv[1];
  token = tokenize(user_input);

  // スコープの初期設定
  scope = calloc(1, sizeof(Scope));
  scope->parent = NULL;
  scope->locals = NULL;
  scope->offset = 0;

  // パース
  program();

  // アセンブリの前半部分を出力
  printf(".intel_syntax noprefix\n");
  Function *cur = funcs; // funcs は連結リストで実装されたスタックなので, 後で登録された関数が先にコード生成される
  while (cur)
  {
    gen_func(cur);
    cur = cur->next;
  }

  return 0;
}