#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "9cc.h"

static Type *derv(Var *var);
static Type *ptr(Var *var);
static Type *seq(Var *var);
static Type *ident(Var *var);
static Type *root();
static Type *strct();

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
  while (consume_reserve("*"))
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

  if (consume_reserve("["))
  {
    Type *newty = calloc(1, sizeof(Type));
    newty->kind = TY_ARRAY;
    int num;
    if (!consume_num(&num))
    {
      num = -1;
    }
    newty->array_size = num;
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

  while (consume_reserve("["))
  {
    Type *newty = calloc(1, sizeof(Type));
    newty->kind = TY_ARRAY;
    newty->array_size = expect_number();
    head->ptr_to = newty;
    head = newty;
    expect("]");
  }
  return ty;
}

Type *ident(Var *var)
{
  if (consume_reserve("("))
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
  if (consume(TK_INT))
  {
    ty = calloc(1, sizeof(Type));
    ty->kind = TY_INT;
    ty->ptr_to = NULL;
  }
  else if (consume(TK_CHAR))
  {
    ty = calloc(1, sizeof(Type));
    ty->kind = TY_CHAR;
    ty->ptr_to = NULL;
  }
  else if (consume(TK_STRUCT))
  {
    ty = strct();
  }
  else
  {
    Tydef *tydef = consume_typedefs();
    if (tydef)
    {
      ty = tydef->type;
    }
  }

  return ty;
}

Type *strct()
{
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = TY_STRUCT;
  ty->ptr_to = NULL;

  Token *tok = consume_ident();
  if (tok)
  {
    Tag *tag = find_tag(tok->str, tok->len);
    if (tag)
    {
      return tag->type;
    }
    else
    {
      tag = calloc(1, sizeof(Tag));
      tag->name = tok->str;
      tag->len = tok->len;
      tag->type = ty;
      add_tag(tag);
    }
  }

  expect("{");
  int nmem = 10;
  Type **members = malloc(10 * sizeof(Type));
  int id = 0;
  int offset = 0;
  for (;;)
  {
    if (consume_reserve("}"))
      break;

    Var var;
    assr(&var);
    expect(";");
    members[id] = var.type;
    members[id]->name = var.name;
    members[id]->len = var.len;

    offset += (size(members[id]) + offset) % size(members[id]) ? size(members[id]) - (size(members[id]) + offset) % size(members[id]) : 0;
    members[id]->offset = offset;
    offset += size(members[id]);

    id++;
    if (id >= nmem)
    {
      nmem *= 2;
      members = realloc(members, nmem);
    }
  }
  members[id] = NULL;
  ty->members = members;
  return ty;
}