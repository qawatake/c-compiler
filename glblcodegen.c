#include <stdlib.h>
#include <stdio.h>
#include "9cc.h"

static int ini_num(Node *node)
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
    return 0;
  }
}

// 構文木から最初に検出した &a の a を返す
// ただし, a はグローバル変数
static GVar *find_addrvar(Node *node)
{
  if (!node)
    return NULL;

  GVar *gvar = NULL;

  if (node->kind == ND_ADDR)
  {
    gvar = find_gvar(node->lhs->name, node->lhs->len);
  }
  else if (node->kind == ND_GVAR)
  {
    if (node->type->kind != TY_ARRAY)
      error("グローバル変数の初期化式で, &がついていない配列でない変数をしようしています");
    gvar = find_gvar(node->name, node->len);
  }
  else if (gvar = find_addrvar(node->lhs))
    ;
  else if (gvar = find_addrvar(node->rhs))
    ;

  return gvar;
}

void gen_gvar(Type *ty, Node *ini)
{
  if (!ini)
  {
    printf("  .zero %d\n", size(ty));
    return;
  }

  // switch 文は内部でローカル変数を宣言できないので, if 文で実装
  if (ty->kind == TY_INT)
  {
    int val = ini_num(ini);
    printf("  .long %d\n", val);
    return;
  }
  if (ty->kind == TY_CHAR)
  {
    int val = ini_num(ini);
    printf("  .byte %d\n", val);
    return;
  }
  if (ty->kind == TY_PTR)
  {
    if (ini->kind == ND_STR)
    {
      printf("  .quad .LC%d\n", ini->serial);
    }
    else
    {
      int val = ini_num(ini);
      GVar *refvar = find_addrvar(ini);
      printf("  .quad ");
      strprint(refvar->name, refvar->len);
      printf("+%d\n", val * size(ty->ptr_to));
    }
    return;
  }
  if (ty->kind == TY_ARRAY)
  {
    if (ini->kind == ND_STR)
    {
      String *str = find_str(ini->serial);
      printf("  .string ");
      strprint(str->body, str->len);
      printf("\n");
    }
    else if (ini->kind == ND_ARRAY)
    {
      for (int id = 0; ini->elems[id]; id++)
      {
        gen_gvar(ty->ptr_to, ini->elems[id]);
      }
    }
    return;
  }

  printf("この型のグローバル変数は初期化できません: ");
  type_tree(ty);
  printf("\n");
  exit(1);
}