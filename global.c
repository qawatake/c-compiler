#include <string.h>
#include <stdlib.h>
#include "9cc.h"

// ローカル変数を保持するために使用しているスタックのオフセット数を返す
static int offset()
{
  Scope *cur = scope;
  while (cur)
  {
    if (cur->locals)
      return cur->locals->offset;
    cur = cur->parent;
  }
  return 0;
}

void add_locals(LVar *lvar)
{
  lvar->next = scope->locals;
  lvar->offset = offset() + size(lvar->type);
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

void add_str(String *str)
{
  str->next = strings;
  strings = str;
}

void add_func(Function *fn)
{
  fn->offset = scope->offset;
  fn->next = funcs;
  funcs = fn;
}

void add_arg(Function *fn)
{
  LVar *cur = scope->locals;
  while (cur)
  {
    LVar *arg = calloc(1, sizeof(LVar));
    arg->name = cur->name;
    arg->len = cur->len;
    arg->type = cur->type;
    arg->offset = cur->offset;

    // ローカル変数は LIFO, つまり, 最新のものが先頭に配置されている
    // 関数の引数は FIFO である必要があるので, 順番を入れ替えて配置している
    arg->next = fn->arg;
    fn->arg = arg;

    cur = cur->next;
  }
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

String *find_str(int serial)
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

int count_strings()
{
  int i = 0;
  String *cur = strings;
  while (cur)
  {
    i++;
    cur = cur->next;
  }
  return i;
}

void zoom_in()
{
  Scope *child = calloc(1, sizeof(Scope));
  child->parent = scope;
  child->locals = NULL;
  scope = child;
}

void zoom_out()
{
  Scope *cur = scope;
  int loffset = (offset() >= scope->offset) ? offset() : scope->offset;
  scope->parent->offset = (loffset >= scope->parent->offset) ? loffset : scope->parent->offset; // 子スコープのうち最大の使用オフセット数に更新
  while (cur->locals != NULL)
  {
    LVar *next = cur->locals->next;
    free(cur->locals);
    cur->locals = next;
  }
  scope = scope->parent;
  free(cur);
}