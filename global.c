#include <string.h>
#include <stdlib.h>
#include "9cc.h"

void add_locals(LVar *lvar)
{
  lvar->next = scope->locals;
  lvar->offset = ((scope->locals) ? scope->locals->offset : scope->offset) + size(lvar->type);
  scope->locals = lvar;
}

void add_globals(GVar *gvar)
{
  gvar->next = globals;
  globals = gvar;
}

void add_typedef(Tydef *tydef)
{
  tydef->next = scope->typedefs;
  scope->typedefs = tydef;
}

void add_tag(Tag *tag)
{
  tag->next = scope->tags;
  scope->tags = tag;
}

LVar *find_lvar(Token *tok)
{
  Scope *cur = scope;

  while (cur)
  {
    LVar *var = cur->locals;
    while (var)
    {
      if (var->len == tok->len && !memcmp(tok->str, var->name, var->len))
      {
        return var;
      }
      var = var->next;
    }
    cur = cur->parent;
  }
  return NULL;
}

GVar *find_gvar(char *name, int len)
{
  GVar *var = globals;
  while (var)
  {
    if (var->len == len && !memcmp(name, var->name, var->len))
      return var;
    var = var->next;
  }
  return NULL;
}

Function *find_func(Token *tok)
{
  Function *cur = funcs;
  while (cur)
  {
    if (strlen(cur->name) == tok->len && !memcmp(tok->str, cur->name, tok->len))
      return cur;
    cur = cur->next;
  }
  return NULL;
}

Tag *find_tag(char *name, int len)
{
  Scope *cur = scope;

  while (cur)
  {
    Tag *tag = cur->tags;
    while (tag)
    {
      if (tag->len == len && !memcmp(name, tag->name, len))
      {
        return tag;
      }
      tag = tag->next;
    }
    cur = cur->parent;
  }
  return NULL;
}

void zoom_in()
{
  Scope *child = calloc(1, sizeof(Scope));
  child->parent = scope;
  child->locals = NULL;
  child->offset = (scope->locals) ? scope->locals->offset : scope->offset;
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
