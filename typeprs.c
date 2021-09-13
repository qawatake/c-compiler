#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "9cc.h"

static Type *derv(Var *var);
static Type *ptr(Var *var);
static Type *seq(Var *var);
static Type *ident(Var *var);
static Type *root();

bool assr(Var *var)
{
  Type *btype = root();
  if (!btype)
    return false;
  Type *ty = derv(var);
  if (ty == NULL)
  {
    ty = btype;
  }
  else
  {
    Type *head = ty;
    while (head->ptr_to)
    {
      head = head->ptr_to;
    }
    head->ptr_to = btype;
  }

  var->type = ty;
  return true;
}

Type *derv(Var *var)
{
  return ptr(var);
}

Type *ptr(Var *var)
{
  Type *parent = NULL;
  while (consume("*"))
  {
    Type *newty = calloc(1, sizeof(Type));
    newty->kind = TY_PTR;
    newty->ptr_to = parent;
    parent = newty;
  }

  Type *ty = seq(var);
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

Type *seq(Var *var)
{
  Type *ty = ident(var);
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

Type *ident(Var *var)
{
  if (consume("("))
  {
    Type *ty = derv(var);
    expect(")");
    return ty;
  }
  Token *tok = consume_ident();
  if (tok)
  {
    var->name = tok->str;
    var->len = tok->len;
    return NULL;
  }

  error("変数宣言が失敗しました");
}

Type *root()
{
  Type *ty = NULL;
  if (consume("int"))
  {
    ty = calloc(1, sizeof(Type));
    ty->kind = TY_INT;
    ty->ptr_to = NULL;
  } else if (consume("char"))
  {
    ty = calloc(1, sizeof(Type));
    ty->kind = TY_CHAR;
    ty->ptr_to = NULL;
  }
  return ty;
}