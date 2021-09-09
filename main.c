#include <stdio.h>
#include "9cc.h"

Token *token;
LVar *locals;
char *user_input;
int lnum = 0;
Function *fns[100];
Scope *scope;

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