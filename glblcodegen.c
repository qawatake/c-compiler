#include <stdlib.h>
#include <stdio.h>
#include "9cc.h"

static int ini_num(Node *node);

void gen_gvar2(GVar *var)
{
  strprint(var->name, var->len);
  printf(":\n");

  if (!(var->ini))
  {

    printf("  .zero %d\n", size(var->type));
    return;
  }

  // switch 文は内部でローカル変数を宣言できないので, if 文で実装
  if (var->type->kind == TY_INT)
  {
    int val = ini_num(var->ini);
    printf("  .long %d\n", val);
    return;
  }
  if (var->type->kind == TY_CHAR)
  {
    int val = ini_num(var->ini);
    printf("  .byte %d\n", val);
    return;
  }

  printf("この型のグローバル変数は初期化できません: ");
  type_tree(var->type);
  printf("\n");
  exit(1);
}

int ini_num(Node *node)
{
  int val;
  switch (node->kind)
  {
  case ND_NUM:
    return node->val;
  case ND_ADD:
    return ini_num(node->lhs) + ini_num(node->rhs);
  case ND_SUB:
    return ini_num(node->lhs) - ini_num(node->rhs);
  case ND_MUL:
    return ini_num(node->lhs) * ini_num(node->rhs);
  case ND_DIV:
    return ini_num(node->lhs) / ini_num(node->rhs);
  case ND_EQ:
    return ini_num(node->lhs) == ini_num(node->rhs);
  case ND_L:
    return ini_num(node->lhs) < ini_num(node->rhs);
  case ND_LE:
    return ini_num(node->lhs) <= ini_num(node->rhs);
  case ND_NE:
    return ini_num(node->lhs) != ini_num(node->rhs);
  default:
    error("NodeKind = %d は int/char 型のグローバル変数の初期化式で処理できません", node->kind);
  }
}