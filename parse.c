#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "9cc.h"

Type *tyjoin(Type *lty, Type *rty)
{
  if (lty->kind > rty->kind)
    return tyjoin(rty, lty);

  if (rty->kind == TY_ARRAY)
  {
    Type *newty = calloc(1, sizeof(Type));
    newty->kind = TY_PTR;
    newty->ptr_to = rty->ptr_to;
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
  case TY_INT_LITERAL:
    return SIZE_INT;
  case TY_INT:
    return SIZE_INT;
  case TY_CHAR:
    return SIZE_CHAR;
  case TY_PTR:
    return SIZE_PTR;
  case TY_ARRAY:
    return (ty->array_size) * size(ty->ptr_to);
  default:
    error("サイズが定義されていません");
  }
}

Node *new_node(NodeKind kind, Node *lhs, Node *rhs)
{
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_node_num(int val)
{
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;

  node->type = calloc(1, sizeof(Type));
  node->type->kind = TY_INT_LITERAL;
  node->type->ptr_to = NULL;
  return node;
}

Node *new_node_lvar(LVar *lvar)
{
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_LVAR;
  node->name = lvar->name;
  node->len = lvar->len;
  node->offset = lvar->offset;
  node->type = lvar->type;
  return node;
}

Node *new_node_gvar(GVar *gvar)
{
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_GVAR;
  node->type = gvar->type;
  node->name = gvar->name;
  node->len = gvar->len;
  return node;
}

Node *new_node_str(String *str)
{
  Node *node = calloc(1, sizeof(Node));
  Type *btype = calloc(1, sizeof(Type));
  Type *ty = calloc(1, sizeof(Type));

  btype->kind = TY_CHAR;
  btype->ptr_to = NULL;
  ty->kind = TY_ARRAY;
  ty->ptr_to = btype;
  ty->array_size = str->len - 1;

  node->kind = ND_STR;
  node->type = ty;
  node->serial = str->serial;
  return node;
}

LVar *newLVar(Var *var)
{
  LVar *lvar = calloc(1, sizeof(LVar));
  lvar->name = var->name;
  lvar->len = var->len;
  lvar->type = var->type;
  return lvar;
}

GVar *newGVar(Var *var)
{
  GVar *gvar = calloc(1, sizeof(GVar));
  gvar->name = var->name;
  gvar->len = var->len;
  gvar->type = var->type;
  return gvar;
}

Tydef *newTydef(Var *var)
{
  Tydef *tydef = calloc(1, sizeof(Tydef));
  tydef->name = var->name;
  tydef->len = var->len;
  tydef->type = var->type;
  return tydef;
}

Function *newFunction(Var *var)
{
  Function *fn = calloc(1, sizeof(Function));
  fn->name = duplicate(var->name, var->len);
  fn->retype = var->type;
  return fn;
}

Node *parse_for_contents()
{
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_FOR;
  expect("(");
  if (consume_reserve(";"))
  {
    node->lhs = new_node(ND_NONE, NULL, NULL);
  }
  else
  {
    node->lhs = expr();
    expect(";");
  }

  if (consume_reserve(";"))
  {
    node->rhs = new_node(ND_WHILE, new_node_num(1), NULL);
  }
  else
  {
    node->rhs = new_node(ND_WHILE, expr(), NULL);
    expect(";");
  }

  if (consume_reserve(")"))
  {
    node->rhs->rhs = new_node(ND_COMP_STMT, NULL, NULL);
  }
  else
  {
    node->rhs->rhs = new_node(ND_COMP_STMT, NULL, expr());
    expect(")");
  }

  node->rhs->rhs->lhs = stmt();
  return node;
}

int parse_array_literal(Node ***nds)
{
  *nds = malloc(10 * sizeof(Node *));
  int size = 10;
  int id = 0;
  expect("{");
  if (consume_reserve("}"))
  {
    (*nds)[0] = NULL;
    return 0;
  }

  (*nds)[id] = assign();
  id++;
  while (consume_reserve(","))
  {
    (*nds)[id] = assign();
    id++;
    if (id >= size)
    {
      size *= 2;
      *nds = realloc(*nds, size * sizeof(Node *));
    }
  }
  expect("}");
  (*nds)[id] = NULL;
  return id;
}

void program()
{
  while (!at_eof())
  {
    Var var;
    assr(&var);
    if (consume_reserve("("))
    {
      zoom_in();

      if (!consume_reserve(")"))
      {
        for (;;)
        {
          Var var;
          assr(&var);
          LVar *lvar = newLVar(&var);
          add_locals(lvar);
          if (consume_reserve(","))
            continue;
          expect(")");
          break;
        }
      }
      if (!check("{"))
        expect("{");

      Function *fn = newFunction(&var);
      add_arg(fn);
      fn->body = stmt();
      zoom_out();
      add_func(fn);
    }
    else
    {
      GVar *gvar = newGVar(&var);
      add_globals(gvar);
      if (consume_reserve("="))
      {
        gvar->ini = equality();
        if (gvar->type->kind == TY_ARRAY && gvar->ini->type->kind == TY_ARRAY && gvar->type->array_size == -1)
        {
          gvar->type->array_size = gvar->ini->type->array_size;
        }
      }
      expect(";");
    }
  }
}

Node *stmt()
{
  Node *node;
  Var var;
  if (consume_reserve("{")) // ブロック文
  {
    zoom_in();
    node = new_node(ND_COMP_STMT, NULL, NULL);
    Node *cur = node;
    while (!consume_reserve("}"))
    {
      cur->lhs = stmt();
      cur->rhs = new_node(ND_COMP_STMT, NULL, NULL);
      cur = cur->rhs;
    }
    cur->kind = ND_NONE;
    zoom_out();
  }
  else if (consume(TK_RETURN))
  {
    node = new_node(ND_RETURN, expr(), NULL);
    expect(";");
  }
  else if (consume(TK_TYPEDEF))
  {
    Var var;
    if (!assr(&var))
      error("typedef の書式に誤りがあります");
    Tydef *tydef = newTydef(&var);
    add_typedef(tydef);
    node = new_node(ND_NONE, NULL, NULL);
    expect(";");
  }
  else if (consume(TK_IF))
  {
    zoom_in();
    node = calloc(1, sizeof(Node));
    node->kind = ND_IF;
    expect("(");
    node->lhs = expr();
    expect(")");
    Node *node_true;
    node_true = stmt();
    if (consume(TK_ELSE))
    {
      Node *node_else = calloc(1, sizeof(Node));
      node_else->kind = ND_ELSE;
      node_else->lhs = node_true;
      node_else->rhs = stmt();
      node->rhs = node_else;
    }
    else
    {
      node->rhs = node_true;
    }
    zoom_out();
  }
  else if (consume(TK_WHILE))
  {
    node = calloc(1, sizeof(Node));
    node->kind = ND_WHILE;
    expect("(");
    node->lhs = expr();
    expect(")");
    node->rhs = stmt();
  }
  else if (consume(TK_FOR))
  {
    zoom_in();
    node = parse_for_contents();
    zoom_out();
  }
  else
  {
    node = expr();
    expect(";");
  }
  return node;
}

Node *expr()
{
  Node *node;
  Var var;
  if (assr(&var))
  {
    LVar *lvar = newLVar(&var);

    if (consume_reserve("="))
    {
      Node *rhs = equality();
      // 右辺の配列の要素数を左辺の配列の要素数として使う
      // lvar と lvar node のオフセットを設定する
      if (lvar->type->kind == TY_ARRAY && rhs->type->kind == TY_ARRAY && lvar->type->array_size == -1)
      {
        lvar->type->array_size = rhs->type->array_size;
      }

      add_locals(lvar);

      Node *lhs = new_node_lvar(lvar);
      node = new_node(ND_ASSIGN, lhs, rhs);
      node->type = lhs->type;
    }
    else
    {
      add_locals(lvar);

      node = new_node(ND_NONE, NULL, NULL);
    }
    return node;
  }
  return assign();
}

Node *assign()
{
  Node *node = equality();

  for (;;)
  {
    if (consume_reserve("="))
    {
      Node *lhs = node;
      Node *rhs = assign();
      node = new_node(ND_ASSIGN, lhs, rhs);
      node->type = lhs->type;
      continue;
    }
    break;
  }
  return node;
}

Node *equality()
{
  Node *node = relational();

  for (;;)
  {
    if (consume_reserve("=="))
    {
      node = new_node(ND_EQ, node, relational());
      node->type = calloc(1, sizeof(Type));
      node->type->kind = TY_INT;
    }
    else if (consume_reserve("!="))
    {
      node = new_node(ND_NE, node, relational());
      node->type = calloc(1, sizeof(Type));
      node->type->kind = TY_INT;
    }
    else
    {
      return node;
    }
  }
}

Node *relational()
{
  Node *node = add();

  for (;;)
  {
    if (consume_reserve(">"))
    {
      node = new_node(ND_L, add(), node);
      node->type = calloc(1, sizeof(Type));
      node->type->kind = TY_INT;
    }
    else if (consume_reserve("<"))
    {
      node = new_node(ND_L, node, add());
      node->type = calloc(1, sizeof(Type));
      node->type->kind = TY_INT;
    }
    else if (consume_reserve(">="))
    {
      node = new_node(ND_LE, add(), node);
      node->type = calloc(1, sizeof(Type));
      node->type->kind = TY_INT;
    }
    else if (consume_reserve("<="))
    {
      node = new_node(ND_LE, node, add());
      node->type = calloc(1, sizeof(Type));
      node->type->kind = TY_INT;
    }
    else
    {
      return node;
    }
  }
}

Node *add()
{
  Node *node = mul();

  for (;;)
  {
    if (consume_reserve("+"))
    {
      Node *rhs = mul();
      Type *ty = tyjoin(node->type, rhs->type);
      node = new_node(ND_ADD, node, rhs);
      node->type = ty;
    }
    else if (consume_reserve("-"))
    {
      Node *rhs = mul();
      Type *ty = tyjoin(node->type, rhs->type);
      node = new_node(ND_SUB, node, rhs);
      node->type = ty;
    }
    else
    {
      return node;
    }
  }
}

Node *mul()
{
  Node *node = unary();

  for (;;)
  {
    if (consume_reserve("*"))
    {
      Node *rhs = unary();
      Type *ty = tyjoin(node->type, rhs->type);
      node = new_node(ND_MUL, node, rhs);
      node->type = ty;
    }
    else if (consume_reserve("/"))
    {
      Node *rhs = unary();
      Type *ty = tyjoin(node->type, rhs->type);
      node = new_node(ND_DIV, node, rhs);
      node->type = ty;
    }
    else
    {
      return node;
    }
  }
}

Node *unary()
{
  if (consume_reserve("-"))
  {
    Node *rhs = unary();
    Node *node = new_node(ND_SUB, new_node_num(0), rhs);
    node->type = rhs->type;
    return node;
  }
  if (consume_reserve("+"))
  {
    return unary();
  }
  if (consume_reserve("&"))
  {
    Node *lhs = unary();
    Node *node = new_node(ND_ADDR, lhs, NULL);
    node->type = calloc(1, sizeof(Type));
    node->type->kind = TY_PTR;
    node->type->ptr_to = lhs->type;
    return node;
  }
  if (consume_reserve("*"))
  {
    Node *lhs = unary();
    Node *node = new_node(ND_DEREF, lhs, NULL);
    node->type = lhs->type->ptr_to;
    return node;
  }
  if (consume(TK_SIZEOF))
  {
    Node *child = unary();
    return new_node_num(size(child->type));
  }
  return comp();
}

Node *parse_func_args()
{
  Node *node;
  Node *cur;
  if (consume_reserve(")"))
  {
    return NULL;
  }
  node = new_node(ND_EXPR_STMT, NULL, expr());
  cur = node;
  while (!consume_reserve(")"))
  {
    expect(",");
    cur->lhs = new_node(ND_EXPR_STMT, NULL, expr());
    cur = cur->lhs;
  }
  return node;
}

Node *new_node_member(Node *parent, Token *tok)
{
  Node *node = new_node(ND_DOT, parent, NULL);
  node->name = tok->str;
  node->len = tok->len;

  Type *strct = parent->type;
  for (int id = 0; strct->members[id]; id++)
  {
    Type *mem = strct->members[id];
    if (mem->len == tok->len && !memcmp(mem->name, tok->str, tok->len))
    {
      node->type = mem;
      return node;
    }
  }
  error("構造体のメンバにアクセスできませんでした");
}

Node *comp()
{
  Node *node = primary();

  for (;;)
  {
    if (consume_reserve("["))
    {
      Node *inside = assign();
      expect("]");
      Type *ty = tyjoin(node->type, inside->type);
      node = new_node(ND_ADD, node, inside);
      node->type = ty;
      node = new_node(ND_DEREF, node, NULL);
      node->type = ty->ptr_to;
    }
    else if (consume_reserve("."))
    {
      Token *tok = consume_ident();
      node = new_node_member(node, tok);
    }
    else if (consume_reserve("->"))
    {
      Token *tok = consume_ident();
      Type *ty = node->type;
      node = new_node(ND_DEREF, node, NULL);
      node->type = ty->ptr_to;
      node = new_node_member(node, tok);
    }
    else
    {
      return node;
    }
  }
}

Node *primary()
{
  // 次のトークンが "(" なら, "(" expr ")" のはず
  if (consume_reserve("("))
  {
    Node *node = expr();
    expect(")");
    return node;
  }

  Token *tok = consume_ident();
  if (tok)
  {
    if (consume_reserve("("))
    {
      Node *node = calloc(1, sizeof(Node));
      Function *fn = find_func(tok);
      if (fn)
      {
        node->type = fn->retype;
      }
      else
      {
        node->type = NULL;
      }
      node->kind = ND_CALL;
      node->lhs = parse_func_args();
      node->name = tok->str;
      node->len = tok->len;
      return node;
    }

    LVar *lvar = find_lvar(tok);
    GVar *gvar = find_gvar(tok->str, tok->len);
    if (lvar)
      return new_node_lvar(lvar);
    else if (gvar)
      return new_node_gvar(gvar);
    else
      error("宣言されていない変数です");
  }

  String *str = consume_str();
  if (str)
  {
    Node *node = new_node_str(str);
    add_str(str);
    return node;
  }

  if (check("{"))
  {
    Node **elems;
    int nelem = parse_array_literal(&elems);

    Type *ty = calloc(1, sizeof(Node));
    ty->kind = TY_ARRAY;
    ty->array_size = nelem;
    ty->ptr_to = (nelem > 0) ? elems[0]->type : NULL;

    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_ARRAY;
    node->type = ty;
    node->elems = elems;
    return node;
  }

  // そうでなければ数値のはず
  return new_node_num(expect_number());
}