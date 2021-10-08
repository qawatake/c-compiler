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

Type *new_type(TypeKind kind)
{
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = kind;
  return ty;
}

Type *new_type_ptr(Type *ptr_to)
{
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = TY_PTR;
  ty->ptr_to = ptr_to;
  return ty;
}

Type *new_type_array(Type *ptr_to, int array_size)
{
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = TY_ARRAY;
  ty->ptr_to = ptr_to;
  ty->array_size = array_size;
  return ty;
}

Type *tyjoin(Type *lty, Type *rty)
{
  if (lty->kind > rty->kind)
    return tyjoin(rty, lty);

  if (rty->kind == TY_ARRAY)
  {
    Type *newty = new_type_ptr(rty->ptr_to);
    return newty;
  }
  else
  {
    return rty;
  }
}

Size size(Type *ty)
{
  if (ty->kind == TY_STRUCT)
  {
    int totalsize = 0;
    int max_mem_size = 0;
    int id = 0;
    while (ty->members[id])
    {
      if (size(ty->members[id]) > max_mem_size)
        max_mem_size = size(ty->members[id]);
      id++;
    }
    if (id)
    {
      totalsize = ty->members[id - 1]->offset + size(ty->members[id - 1]);
      totalsize += (max_mem_size + totalsize) % max_mem_size ? max_mem_size - (max_mem_size + totalsize) % max_mem_size : 0;
    }
    return totalsize;
  }

  switch (ty->kind)
  {
  case TY_INT:
  case TY_INT_LITERAL:
    return SIZE_INT;
  case TY_CHAR:
  case TY_VOID:
    return SIZE_CHAR;
  case TY_PTR:
    return SIZE_PTR;
  case TY_ARRAY:
    return (ty->array_size) * size(ty->ptr_to);
  default:
    error("サイズが定義されていません");
  }
}

bool assr(Var *var)
{
  var->name = NULL;
  var->len = 0;

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
    Type *newty = new_type_ptr(parent);
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
    int num;
    if (!consume_num(&num))
    {
      num = -1;
    }
    Type *newty = new_type_array(NULL, num);
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
    Type *newty = new_type_array(NULL, expect_number());
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
  return NULL;
  // error("変数宣言が失敗しました");
}

Type *root()
{
  Type *ty = NULL;
  if (consume(TK_INT))
  {
    ty = new_type(TY_INT);
  }
  else if (consume(TK_CHAR))
  {
    ty = new_type(TY_CHAR);
  }
  else if (consume(TK_STRUCT))
  {
    ty = strct();
  }
  else if (consume(TK_VOID))
  {
    ty = new_type(TY_VOID);
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
  Type *ty = new_type(TY_STRUCT);
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