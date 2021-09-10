#include <stdio.h>
#include <stdlib.h>
#include "9cc.h"

static Type *derv(LVar *lvar);
static Type *ptr(LVar *lvar);
static Type *seq(LVar *lvar);
static Type *ident(LVar *lvar);
static Type *root();

void *assr(Type *root, LVar *lvar)
{
  Type *ty = derv(lvar);
  if (ty == NULL)
  {
    ty = root;
  }
  else
  {
    Type *head = ty;
    while (head->ptr_to)
    {
      head = head->ptr_to;
    }
    head->ptr_to = root;
  }

  lvar->type = ty;
}

Type *derv(LVar *lvar)
{
  return ptr(lvar);
}

Type *ptr(LVar *lvar)
{
  Type *parent = NULL;
  while (consume("*"))
  {
    Type *newty = calloc(1, sizeof(Type));
    newty->kind = TY_PTR;
    newty->ptr_to = parent;
    parent = newty;
  }

  Type *ty = seq(lvar);
  if (ty == NULL)
  {
    ty = parent;
  }
  else
  {
    Type *head = ty;
    while (head->ptr_to)
    {
      head = head->ptr_to;
    }
    head->ptr_to = parent;
  }
  return ty;
}

Type *seq(LVar *lvar)
{
  Type *ty = ident(lvar);
  Type *head = ty;
  if (head)
  {
    while (head->ptr_to)
    {
      head = head->ptr_to;
    }
  }

  while (consume("["))
  {
    Type *newty = calloc(1, sizeof(Type));
    newty->kind = TY_ARRAY;
    newty->array_size = expect_number();
    if (ty == NULL)
    {
      ty = newty;
      head = ty;
    }
    else
    {
      head->ptr_to = newty;
      head = newty;
    }
    expect("]");
  }
  return ty;
}

Type *ident(LVar *lvar)
{
  if (consume("("))
  {
    Type *ty = derv(lvar);
    expect(")");
    return ty;
  }

  Token *tok = consume_ident();
  if (tok)
  {
    if (find_lvar(scope, tok))
      error("すでに宣言された変数です");
    lvar->name = tok->str;
    lvar->len = tok->len;
    return NULL;
  }

  error("変数宣言が失敗しました");
}