#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "9cc.h"

void error_at(char *loc, char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);

  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, " ");
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

void error(char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

char *duplicate(char *str, size_t len)
{

  char *buffer = malloc(len + 1);
  memcpy(buffer, str, len);
  buffer[len] = '\0';

  return buffer;
}

void strprint(char *str, size_t len)
{
  for (int i = 0; i < len; i++)
  {
    printf("%c", str[i]);
  }
}

int scpdepth(Scope *scp)
{
  int i = 0;
  while (scp)
  {
    scp = scp->parent;
    i++;
  }
  return i;
}

void syntax_tree(int depth, Node *node)
{
  if (!node)
    return;

  char *ndkind;

  switch (node->kind)
  {
  case ND_ADD:
    ndkind = "+";
    break;
  case ND_SUB:
    ndkind = "-";
    break;
  case ND_MUL:
    ndkind = "*";
    break;
  case ND_DIV:
    ndkind = "/";
    break;
  case ND_EQ:
    ndkind = "==";
    break;
  case ND_NE:
    ndkind = "!=";
    break;
  case ND_L:
    ndkind = "<";
    break;
  case ND_LE:
    ndkind = "<=";
    break;
  case ND_ASSIGN:
    ndkind = "=";
    break;
  case ND_NONE:
    return;
  case ND_COMP_STMT:
    syntax_tree(depth, node->lhs);
    syntax_tree(depth, node->rhs);
    return;
  case ND_EXPR_STMT:
    syntax_tree(depth, node->rhs);
    syntax_tree(depth, node->lhs);
    return;
  case ND_CALL:
    ndkind = "CALL";
    break;
  case ND_RETURN:
    ndkind = "return";
    break;
  case ND_IF:
    ndkind = "if";
    break;
  case ND_ELSE:
    ndkind = "else";
    break;
  case ND_WHILE:
    ndkind = "while";
    break;
  case ND_FOR:
    ndkind = "for";
    break;
  case ND_ADDR:
    ndkind = "&";
    break;
  case ND_DEREF:
    ndkind = "*X";
    break;
  case ND_FUNC:
    ndkind = "FUNC";
    break;
  case ND_LVAR:
    ndkind = "LVAR";
    break;
  case ND_NUM:
    ndkind = "NUM";
    break;
  default:
    error("デバッグ: '%d' は登録されていない NodeKInd です", node->kind);
  }

  for (int i = 0; i < depth; i++)
  {
    printf("  ");
  }

  printf("|- %s: ", ndkind);
  if (node->kind == ND_LVAR || node->kind == ND_CALL)
  {
    for (int i = 0; i < node->len; i++)
    {
      printf("%c", node->name[i]);
    }
  }
  if (node->kind == ND_NUM)
    printf("%d", node->val);

  printf("\n");

  if (node->lhs)
    syntax_tree(depth + 1, node->lhs);
  if (node->rhs)
    syntax_tree(depth + 1, node->rhs);
}