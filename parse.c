#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "9cc.h"


int tycmp(Type *lty, Type *rty)
{
  if (lty == NULL)
    return 1;
  if (rty == NULL)
    return -1;

  switch (lty->kind)
  {
  case TY_INT:
    if (rty->kind == TY_INT_LITERAL)
      return -1;
    if (rty->kind == TY_INT)
      return 0;
    if (rty->kind == TY_PTR || rty->kind == TY_ARRAY)
      return 1;
    error("両辺の型が不整合です");
  case TY_INT_LITERAL:
    if (rty->kind == TY_INT_LITERAL)
      return 0;
    if (rty->kind == TY_INT || rty->kind == TY_PTR || rty->kind == TY_ARRAY)
      return 1;
    error("両辺の型が不整合です");
  case TY_ARRAY:
    if (rty->kind == TY_INT_LITERAL || rty->kind == TY_INT)
      return -1;
    if (rty->kind == TY_ARRAY)
      return 0;
    if (rty->kind == TY_PTR)
      return 1;
  case TY_PTR:
    if (rty->kind == TY_INT_LITERAL || rty->kind == TY_INT || rty->kind == TY_ARRAY)
      return -1;
    if (rty->kind == TY_PTR)
    {
      return tycmp(lty->ptr_to, rty->ptr_to);
    }
    error("両辺の型が不整合です");
  }
}

Type *tyjoin(Type *lty, Type *rty)
{
  if (tycmp(lty, rty) > 0)
    return tyjoin(rty, lty);

  if (lty->kind != TY_ARRAY)
    return lty;

  Type *newty = calloc(1, sizeof(Type));
  newty->kind = TY_PTR;
  newty->ptr_to = lty->ptr_to;
  return newty;
}

Size size(Type *ty)
{
  switch (ty->kind)
  {
  case TY_INT_LITERAL:
    return SIZE_INT;
  case TY_INT:
    return SIZE_INT;
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

Node *parse_for_contents()
{
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_FOR;
  expect("(");
  if (consume(";"))
  {
    node->lhs = new_node(ND_NONE, NULL, NULL);
  }
  else
  {
    node->lhs = expr();
    expect(";");
  }

  if (consume(";"))
  {
    node->rhs = new_node(ND_WHILE, new_node_num(1), NULL);
  }
  else
  {
    node->rhs = new_node(ND_WHILE, expr(), NULL);
    expect(";");
  }

  if (consume(")"))
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

void zoom_in()
{
  Scope *child = calloc(1, sizeof(Scope));
  child->parent = scope;
  child->locals = NULL;
  child->offset = 0;
  scope = child;
}

void zoom_out()
{
  Scope *cur = scope;
  int loffset = (scope->locals) ? scope->locals->offset : 0;
  scope->parent->offset = (loffset + scope->offset >= scope->parent->offset) ? (loffset + scope->offset) : scope->parent->offset; // 子スコープのうち最大の使用メモリ数に更新
  while (cur->locals != NULL)
  {
    LVar *next = cur->locals->next;
    free(cur->locals);
    cur->locals = next;
  }
  scope = scope->parent;
  free(cur);
}

void program()
{
  scope = calloc(1, sizeof(Scope));
  scope->parent = NULL;
  scope->locals = NULL;
  scope->offset = 0;
  int i = 0;
  fns[0] = NULL;
  while (!at_eof())
  {
    fns[i] = function();
    fns[i]->offset = scope->offset;
    i++;
  }
  fns[i] = NULL;
}

Function *function()
{
  if (!consume("int"))
  {
    error("関数宣言の冒頭に型名がありません");
  }

  Type *rety = calloc(1, sizeof(Type));
  rety->kind = TY_INT;
  rety->ptr_to = NULL;
  // 返り値の型をパースする
  while (consume("*"))
  {
    Type *newty = calloc(1, sizeof(Type));
    newty->kind = TY_PTR;
    newty->ptr_to = rety;
    rety = newty;
  }

  Token *tok = consume_ident();
  if (!tok)
  {
    error("関数名ではありません");
  }
  Function *fn = calloc(1, sizeof(Function));
  fn->name = duplicate(tok->str, tok->len);
  fn->retype = rety;
  zoom_in();

  expect("(");
  if (!consume(")"))
  {
    for (;;)
    {
      if (!consume("int"))
        error("関数の引数の冒頭に型名がありません");

      Type *ty = calloc(1, sizeof(Type));
      ty->kind = TY_INT;
      ty->ptr_to = NULL;
      while (consume("*"))
      {
        Type *newty = calloc(1, sizeof(Type));
        newty->kind = TY_PTR;
        newty->ptr_to = ty;
        ty = newty;
      }

      Token *tok = consume_ident();
      LVar *lvar = calloc(1, sizeof(LVar));
      lvar->next = scope->locals;
      lvar->name = tok->str;
      lvar->len = tok->len;
      lvar->type = ty;
      lvar->offset = (scope->locals) ? scope->locals->offset + 8 : 8;
      scope->locals = lvar;
      if (consume(","))
        continue;
      expect(")");
      break;
    }
  }
  expect("{");

  if (consume("}"))
  {
    fn->body = new_node(ND_NONE, NULL, NULL);
    zoom_out();
    return fn;
  }
  fn->body = new_node(ND_COMP_STMT, stmt(), NULL);

  Node *cur = fn->body;
  while (!consume("}"))
  {
    cur->rhs = new_node(ND_COMP_STMT, stmt(), NULL);
    cur = cur->rhs;
  }
  cur->rhs = new_node(ND_NONE, NULL, NULL);
  zoom_out();
  return fn;
}

void var_assertion(Type *btype)
{
  Type *cur = btype;
  while (consume("*"))
  {
    Type *newty = calloc(1, sizeof(Type));
    newty->kind = TY_PTR;
    newty->ptr_to = cur;
    cur = newty;
  }

  Token *tok = consume_ident();
  if (!tok)
  {
    error("変数名がありません");
  }

  LVar *lvar = find_lvar(tok);
  if (lvar)
  {
    error("すでに宣言された変数です");
  }

  lvar = calloc(1, sizeof(LVar));
  lvar->next = scope->locals;
  lvar->name = tok->str;
  lvar->len = tok->len;

  if (consume("["))
  {
    Type *newty = calloc(1, sizeof(Type));
    newty->kind = TY_ARRAY;
    newty->ptr_to = cur;
    newty->array_size = expect_number();
    cur = newty;
    expect("]");
    lvar->type = cur;
    lvar->offset = (scope->locals) ? scope->locals->offset + size(lvar->type) + 8 : size(lvar->type) + 8;
    scope->locals = lvar;
  }
  else
  {
    lvar->type = cur;
    lvar->offset = (scope->locals) ? scope->locals->offset + size(lvar->type) : size(lvar->type);
    scope->locals = lvar;
  }
}

Node *stmt()
{
  Node *node;
  if (consume("{"))
  {
    node = new_node(ND_COMP_STMT, NULL, NULL);
    Node *cur = node;
    while (!consume("}"))
    {
      cur->lhs = stmt();
      cur->rhs = new_node(ND_COMP_STMT, NULL, NULL);
      cur = cur->rhs;
    }
    cur->kind = ND_NONE;
  }
  else if (consume("return"))
  {
    node = calloc(1, sizeof(Node));
    node->kind = ND_RETURN;
    node->lhs = expr();
    expect(";");
  }
  else if (consume("int"))
  {
    node = calloc(1, sizeof(Node));
    node->kind = ND_NONE;

    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_INT;
    ty->ptr_to = NULL;

    var_assertion(ty);

    expect(";");
  }
  else if (consume("if"))
  {
    node = calloc(1, sizeof(Node));
    node->kind = ND_IF;
    expect("(");
    node->lhs = expr();
    expect(")");
    Node *node_true;
    node_true = stmt();
    if (consume("else"))
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
  }
  else if (consume("while"))
  {
    node = calloc(1, sizeof(Node));
    node->kind = ND_WHILE;
    expect("(");
    node->lhs = expr();
    expect(")");
    node->rhs = stmt();
  }
  else if (consume("for"))
  {
    node = parse_for_contents();
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
  return assign();
}

Node *assign()
{
  Node *node = equality();

  for (;;)
  {
    if (consume("="))
    {
      Node *rhs = assign();
      Type *ty = tyjoin(node->type, rhs->type);
      node = new_node(ND_ASSIGN, node, rhs);
      node->type = ty;
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
    if (consume("=="))
    {
      node = new_node(ND_EQ, node, relational());
      node->type = calloc(1, sizeof(Type));
      node->type->kind = TY_INT;
    }
    else if (consume("!="))
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
    if (consume(">"))
    {
      node = new_node(ND_L, add(), node);
      node->type = calloc(1, sizeof(Type));
      node->type->kind = TY_INT;
    }
    else if (consume("<"))
    {
      node = new_node(ND_L, node, add());
      node->type = calloc(1, sizeof(Type));
      node->type->kind = TY_INT;
    }
    else if (consume(">="))
    {
      node = new_node(ND_LE, add(), node);
      node->type = calloc(1, sizeof(Type));
      node->type->kind = TY_INT;
    }
    else if (consume("<="))
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
    if (consume("+"))
    {
      Node *rhs = mul();
      Type *ty = tyjoin(node->type, rhs->type);
      node = new_node(ND_ADD, node, rhs);
      node->type = ty;
    }
    else if (consume("-"))
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
    if (consume("*"))
    {
      Node *rhs = unary();
      Type *ty = tyjoin(node->type, rhs->type);
      node = new_node(ND_MUL, node, rhs);
      node->type = ty;
    }
    else if (consume("/"))
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
  if (consume("-"))
  {
    Node *rhs = unary();
    Node *node = new_node(ND_SUB, new_node_num(0), rhs);
    node->type = rhs->type;
    return node;
  }
  if (consume("+"))
  {
    return unary();
  }
  if (consume("&"))
  {
    Node *lhs = unary();
    Node *node = new_node(ND_ADDR, lhs, NULL);
    node->type = calloc(1, sizeof(Type));
    node->type->kind = TY_PTR;
    node->type->ptr_to = lhs->type;
    return node;
  }
  if (consume("*"))
  {
    Node *lhs = unary();
    Node *node = new_node(ND_DEREF, lhs, NULL);
    node->type = lhs->type->ptr_to;
    return node;
  }
  if (consume("sizeof"))
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
  if (consume(")"))
  {
    return NULL;
  }
  node = new_node(ND_EXPR_STMT, NULL, expr());
  cur = node;
  while (!consume(")"))
  {
    expect(",");
    cur->lhs = new_node(ND_EXPR_STMT, NULL, expr());
    cur = cur->lhs;
  }
  return node;
}

Function *find_func(Token *tok)
{
  for (int i = 0; fns[i]; i++)
  {
    if (strlen(fns[i]->name) == tok->len && !memcmp(tok->str, fns[i]->name, tok->len))
    {
      return fns[i];
    }
  }
  return NULL;
}

Node *comp()
{
  Node *node = primary();

  for (;;)
  {
    if (consume("["))
    {
      Node *inside = expr();
      expect("]");
      Type *ty = tyjoin(node->type, inside->type);
      node = new_node(ND_ADD, node, inside);
      node->type = ty;
      node = new_node(ND_DEREF, node, NULL);
      node->type = ty->ptr_to;
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
  if (consume("("))
  {
    Node *node = expr();
    expect(")");
    return node;
  }

  Token *tok = consume_ident();
  if (tok)
  {
    Node *node = calloc(1, sizeof(Node));
    if (consume("("))
    {
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

    node->kind = ND_LVAR;
    LVar *lvar = find_lvar(tok);
    if (!lvar)
    {
      error("宣言されていない変数です");
    }

    node->offset = lvar->offset;
    node->type = lvar->type;
    return node;
  }

  // そうでなければ数値のはず
  return new_node_num(expect_number());
}