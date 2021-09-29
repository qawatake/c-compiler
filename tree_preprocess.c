#include <stdlib.h>
#include <stdio.h>
#include "9cc.h"

static String *find_str(int serial)
{
  String *cur = strings;
  while (cur)
  {
    if (cur->serial == serial)
      return cur;
    cur = cur->next;
  }
  return NULL;
}

static void str_to_array(Type *ty, Node **pnode)
{
  Node *node = *pnode;

  if (node->kind == ND_STR && ty->kind == TY_ARRAY)
  {
    // 文字列ノードを配列リテラルノードに変更
    String *str = find_str(node->serial);
    Node *nd = calloc(1, sizeof(Node));
    nd->kind = ND_ARRAY;
    nd->type = ty;
    nd->elems = malloc((str->len - 1) * sizeof(Node *));
    for (int i = 0; i < str->len - 2; i++)
    {
      nd->elems[i] = new_node_num(str->body[i + 1]);
    }
    nd->elems[str->len - 2] = new_node_num(0);
    *pnode = nd;
    return;
  }

  if (node->kind == ND_ARRAY)
  {
    for (int id = 0; node->elems[id]; id++)
    {
      str_to_array(ty->ptr_to, &(node->elems[id]));
    }
  }
}

// 初期化式
// 右辺にある文字列は, 左辺の型が配列であれば, 配列として認識される
// 左辺の型がポインタであれば, 文字列の第0要素へのポインタとして認識される (つまり, 通常の扱い)
static void walk_str_to_array(Node **pnode)
{
  Node *node = *pnode;
  if (!node)
  {
    return;
  }

  if (node->kind == ND_ASSIGN && node->type)
  {
    str_to_array(node->lhs->type, &(node->rhs));
    return;
  }

  if (node->lhs)
    walk_str_to_array(&(node->lhs));
  if (node->rhs)
    walk_str_to_array(&(node->rhs));
}


// 構文木をたどって, 配列初期化を表す枝を入れ替えていく
// int x[2][2] = {{1, 2}, {3, 4}};
// =>
// int x[2][2];
// x[0] = {1, 2};
// x[1] = {3, 4};
// =>
// int x[2][2];
// x[0][0] = 1;
// x[0][1] = 2;
// x[1][0] = 3;
// x[1][1] = 4;
static void walk_array_expansion(Node **pnode)
{
  Node *node = *pnode;
  if (!node)
    return;

  if (node->kind == ND_ASSIGN && node->type && node->type->kind == TY_ARRAY)
  {
    Node *lnode = node->lhs;
    Node **elems = node->rhs->elems;
    int nelem = node->rhs->type->array_size;

    *pnode = new_node(ND_COMP_STMT, NULL, NULL);
    node = *pnode;
    Node *cur = *pnode;
    for (int i = 0; i < lnode->type->array_size; i++)
    {
      if (i >= lnode->type->array_size)
        error("配列初期化式で右辺の要素数が左辺の要素数より大きくなっています");
      Node *nd;
      Node *lhs = lnode;
      Node *rhs = new_node_num(i);
      Type *ty = tyjoin(lhs->type, rhs->type);
      nd = new_node(ND_ADD, lhs, rhs);
      nd->type = ty;

      lhs = nd;
      ty = lhs->type;
      nd = new_node(ND_DEREF, lhs, NULL);
      nd->type = ty->ptr_to;

      lhs = nd;
      if (i < nelem)
        rhs = elems[i];
      else
        rhs = new_node_num(0);
      nd = new_node(ND_ASSIGN, lhs, rhs);
      nd->type = lhs->type;

      cur->lhs = nd;
      cur->rhs = new_node(ND_COMP_STMT, NULL, NULL);
      cur = cur->rhs;
    }
    cur->kind = ND_NONE;
  }

  if (node->lhs)
    walk_array_expansion(&(node->lhs));
  if (node->rhs)
    walk_array_expansion(&(node->rhs));
}

void preprocess(Node **pnode)
{
  walk_str_to_array(pnode);
  walk_array_expansion(pnode);
}