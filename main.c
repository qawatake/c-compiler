#include <stdio.h>
#include <stdlib.h>
#include "9cc.h"

Token *token;
char *user_input;
int lnum = 0;
Scope *scope;
Function *funcs;
GVar *globals;
String *strings;

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

  // 文字列の初期化
  strings = NULL;

  // パース
  program();

  // プリプロセッサ (構文木の再構成)
  Function *cur = funcs;
  while (cur)
  {
    str_to_array(&(cur->body));
    array_expansion(&(cur->body));
    cur = cur->next;
  }

  // コード生成
  gen_x86();

  return 0;
}