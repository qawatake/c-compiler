#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "9cc.h"

static Node *array_literal();
static Node *for_sentence();
static Node *args();
static Node *if_else();

Node *new_node(NodeKind kind, Node *lhs, Node *rhs, Type *ty)
{
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  node->type = ty;
  return node;
}

Node *new_node_num(int val)
{
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;

  node->type = new_type(TY_INT_LITERAL);
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
  Type *btype = new_type(TY_CHAR);
  Type *ty = new_type_array(btype, str->len - 1);

  Node *node = new_node(ND_STR, NULL, NULL, ty);
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

Node *for_sentence()
{
  consume(TK_FOR);
  Node *node = new_node(ND_FOR, NULL, NULL, NULL);
  expect("(");
  if (consume_reserve(";"))
  {
    node->lhs = new_node(ND_NONE, NULL, NULL, NULL);
  }
  else
  {
    node->lhs = expr();
    expect(";");
  }

  if (consume_reserve(";"))
  {
    node->rhs = new_node(ND_WHILE, new_node_num(1), NULL, NULL);
  }
  else
  {
    node->rhs = new_node(ND_WHILE, expr(), NULL, NULL);
    expect(";");
  }

  if (consume_reserve(")"))
  {
    node->rhs->rhs = new_node(ND_COMP_STMT, NULL, NULL, NULL);
  }
  else
  {
    node->rhs->rhs = new_node(ND_COMP_STMT, NULL, expr(), NULL);
    expect(")");
  }

  node->rhs->rhs->lhs = stmt();
  return node;
}

Node *if_else()
{
  consume(TK_IF);
  Node *node = new_node(ND_IF, NULL, NULL, NULL);
  expect("(");
  node->lhs = expr();
  expect(")");
  Node *node_true;
  node_true = stmt();
  if (consume(TK_ELSE))
  {
    Node *node_else = new_node(ND_ELSE, node_true, stmt(), NULL);
    node->rhs = node_else;
  }
  else
  {
    node->rhs = node_true;
  }
  return node;
}

Node *array_literal()
{
  Node *node;
  Node **elems = malloc(10 * sizeof(Node *));
  int size = 10;
  int id = 0;
  expect("{");
  if (consume_reserve("}"))
  {
    elems[0] = NULL;
    return new_node(ND_ARRAY, NULL, NULL, new_type_array(NULL, 0));
  }

  elems[0] = assign();
  id++;
  while (consume_reserve(","))
  {
    elems[id] = assign();
    id++;
    if (id >= size)
    {
      size *= 2;
      elems = realloc(elems, size * sizeof(Node *));
    }
  }
  expect("}");

  node = new_node(ND_ARRAY, NULL, NULL, new_type_array(elems[0]->type, id));
  node->elems = elems;
  return node;
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
      if (!check_reserve("{"))
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
    node = new_node(ND_COMP_STMT, NULL, NULL, NULL);
    Node *cur = node;
    while (!consume_reserve("}"))
    {
      cur->lhs = stmt();
      cur->rhs = new_node(ND_COMP_STMT, NULL, NULL, NULL);
      cur = cur->rhs;
    }
    cur->kind = ND_NONE;
    zoom_out();
  }
  else if (consume(TK_RETURN))
  {
    node = new_node(ND_RETURN, expr(), NULL, NULL);
    expect(";");
  }
  else if (consume(TK_TYPEDEF))
  {
    Var var;
    if (!assr(&var))
      error("typedef の書式に誤りがあります");
    Tydef *tydef = newTydef(&var);
    add_typedef(tydef);
    node = new_node(ND_NONE, NULL, NULL, NULL);
    expect(";");
  }
  else if (check(TK_IF))
  {
    zoom_in();
    node = if_else();
    zoom_out();
  }
  else if (consume(TK_WHILE))
  {
    node = new_node(ND_WHILE, NULL, NULL, NULL);
    expect("(");
    node->lhs = expr();
    expect(")");
    node->rhs = stmt();
  }
  else if (check(TK_FOR))
  {
    zoom_in();
    node = for_sentence();
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
      node = new_node(ND_ASSIGN, lhs, rhs, lhs->type);
    }
    else
    {
      add_locals(lvar);

      node = new_node(ND_NONE, NULL, NULL, NULL);
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
      node = new_node(ND_ASSIGN, lhs, rhs, lhs->type);
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
    Type *ty = new_type(TY_CHAR);
    if (consume_reserve("=="))
    {
      node = new_node(ND_EQ, node, relational(), ty);
    }
    else if (consume_reserve("!="))
    {
      node = new_node(ND_NE, node, relational(), ty);
    }
    else
    {
      free(ty);
      return node;
    }
  }
}

Node *relational()
{
  Node *node = add();

  for (;;)
  {
    Type *ty = new_type(TY_CHAR);
    if (consume_reserve(">"))
    {
      node = new_node(ND_L, add(), node, ty);
    }
    else if (consume_reserve("<"))
    {
      node = new_node(ND_L, node, add(), ty);
    }
    else if (consume_reserve(">="))
    {
      node = new_node(ND_LE, add(), node, ty);
    }
    else if (consume_reserve("<="))
    {
      node = new_node(ND_LE, node, add(), ty);
    }
    else
    {
      free(ty);
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
      node = new_node(ND_ADD, node, rhs, tyjoin(node->type, rhs->type));
    }
    else if (consume_reserve("-"))
    {
      Node *rhs = mul();
      node = new_node(ND_SUB, node, rhs, tyjoin(node->type, rhs->type));
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
      node = new_node(ND_MUL, node, rhs, tyjoin(node->type, rhs->type));
    }
    else if (consume_reserve("/"))
    {
      Node *rhs = unary();
      node = new_node(ND_DIV, node, rhs, tyjoin(node->type, rhs->type));
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
    Node *node = new_node(ND_SUB, new_node_num(0), rhs, rhs->type);
    return node;
  }
  if (consume_reserve("+"))
  {
    return unary();
  }
  if (consume_reserve("&"))
  {
    Node *lhs = unary();
    Type *ty = new_type_ptr(lhs->type);
    Node *node = new_node(ND_ADDR, lhs, NULL, ty);
    return node;
  }
  if (consume_reserve("*"))
  {
    Node *lhs = unary();
    Node *node = new_node(ND_DEREF, lhs, NULL, lhs->type->ptr_to);
    return node;
  }
  if (consume(TK_SIZEOF))
  {
    Node *child = unary();
    return new_node_num(size(child->type));
  }
  return comp();
}

Node *args()
{
  Node *node;
  Node *cur;
  consume_reserve("(");
  if (consume_reserve(")"))
    return NULL;
  node = new_node(ND_EXPR_STMT, NULL, expr(), NULL);
  cur = node;
  while (!consume_reserve(")"))
  {
    expect(",");
    cur->lhs = new_node(ND_EXPR_STMT, NULL, expr(), NULL);
    cur = cur->lhs;
  }
  return node;
}

Node *new_node_member(Node *parent, Token *tok)
{
  Node *node = new_node(ND_DOT, parent, NULL, NULL);
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
      node = new_node(ND_ADD, node, inside, ty);
      node = new_node(ND_DEREF, node, NULL, ty->ptr_to);
    }
    else if (consume_reserve("."))
    {
      Token *tok = consume_ident();
      node = new_node_member(node, tok);
    }
    else if (consume_reserve("->"))
    {
      Token *tok = consume_ident();
      node = new_node(ND_DEREF, node, NULL, node->type->ptr_to);
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
    // 関数呼び出し
    if (check_reserve("("))
    {
      Function *fn = find_func(tok);
      Type *ty = fn ? fn->retype : NULL;
      Node *node = new_node(ND_CALL, args(), NULL, ty);
      node->name = tok->str;
      node->len = tok->len;
      return node;
    }

    // ローカル変数
    LVar *lvar = find_lvar(tok);
    if (lvar)
      return new_node_lvar(lvar);

    // グローバル変数
    GVar *gvar = find_gvar(tok->str, tok->len);
    if (gvar)
      return new_node_gvar(gvar);
    error("宣言されていない変数です");
  }

  // 文字列リテラル
  String *str = consume_str();
  if (str)
  {
    Node *node = new_node_str(str);
    add_str(str);
    return node;
  }

  // 配列リテラル
  if (check_reserve("{"))
    return array_literal();

  // そうでなければ数値のはず
  return new_node_num(expect_number());
}