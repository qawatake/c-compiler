#include <stdlib.h>
#include <stdio.h>
#include "9cc.h"

void array_expansion(Node **pnode)
{
  Node *node = *pnode;
  if (!node)
    return;

  if (node->kind == ND_ASSIGN && node->type && node->type->kind == TY_ARRAY)
  {
    Node *lnode = node->lhs;
    Node **elems = node->rhs->elems;
    int nelem = node->rhs->type->array_size;
    if (lnode->type->array_size < 0)
      lnode->type->array_size = nelem;

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
      // printf("rrrr\n");
      Type *ty = tyjoin(lhs->type, rhs->type);
      // printf("eeee\n");
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
      // printf("1111\n");
      // ty = tyjoin(lhs->type, rhs->type);
      // printf("0000\n");
      nd = new_node(ND_ASSIGN, lhs, rhs);
      nd->type = lhs->type;

      cur->lhs = nd;
      cur->rhs = new_node(ND_COMP_STMT, NULL, NULL);
      cur = cur->rhs;
    }
    cur->kind = ND_NONE;
  }

  if (node->lhs)
    array_expansion(&(node->lhs));
  if (node->rhs)
    array_expansion(&(node->rhs));
}