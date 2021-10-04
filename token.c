#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "9cc.h"

bool consume(char *op)
{
  if (token->kind != TK_RESERVED && token->kind != TK_RETURN && token->kind != TK_IF && token->kind != TK_ELSE && token->kind != TK_WHILE && token->kind != TK_FOR && token->kind != TK_INT && token->kind != TK_SIZEOF && token->kind != TK_CHAR && token->kind != TK_STRUCT)
    return false;
  else if (token->len != strlen(op) || memcmp(token->str, op, token->len))
    return false;
  token = token->next;
  return true;
}

Token *consume_ident()
{
  if (token->kind == TK_IDENT)
  {
    Token *tok = token;
    token = token->next;
    return tok;
  }
  return NULL;
}

String *consume_str()
{
  if (token->kind != TK_STR)
    return NULL;
  String *str = calloc(1, sizeof(String));
  str->body = token->str;
  str->len = token->len;
  str->next = strings;
  str->serial = count_strings();
  token = token->next;
  return str;
}

bool consume_num(int *num)
{
  if (token->kind != TK_NUM)
    return false;
  *num = token->val;
  token = token->next;
  return true;
}

bool check(char *op)
{
  if (token->kind != TK_RESERVED || token->len != strlen(op) || memcmp(token->str, op, token->len))
    return false;
  return true;
}

void expect(char *op)
{
  if (token->kind != TK_RESERVED || token->len != strlen(op) || memcmp(token->str, op, token->len))
    error_at(token->str, "'%s'ではありません", op);
  token = token->next;
}

int expect_number()
{
  if (token->kind != TK_NUM)
    error_at(token->str, "数ではありません");
  int val = token->val;
  token = token->next;
  return val;
}

bool at_eof()
{
  return token->kind == TK_EOF;
}

// 新しいトークンを作成して cur に繋げる
Token *new_token(TokenKind kind, Token *cur, char *str, int len)
{
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

bool startswith(char *p, char *q)
{
  return memcmp(p, q, strlen(q)) == 0;
}

bool is_alnum(char c)
{
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9') || (c == '_');
}

// 入力文字列 p をトークナイズしてそれを返す
Token *tokenize(char *p)
{
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p)
  {
    // 空白文字をスキップ
    if (isspace(*p))
    {
      p++;
      continue;
    }

    // 行コメント
    if (startswith(p, "//"))
    {
      p += 2;
      while (*p != '\n')
        p++;
      continue;
    }

    // ブロックコメント
    if (startswith(p, "/*"))
    {
      p += 2;
      while (!startswith(p, "*/"))
      {
        p++;
      }
      p += 2;
      continue;
    }

    // Multi-letter punctuator
    if (startswith(p, "==") || startswith(p, "!=") || startswith(p, ">=") || startswith(p, "<="))
    {
      cur = new_token(TK_RESERVED, cur, p, 2);
      p += 2;
      continue;
    }

    // Single-letter punctuator
    if (strchr("+-*/()<>=;{},&[].", *p))
    {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }

    if (*p == '\"')
    {
      int i = 1; // " を含む文字列全体の長さ
      while (p[i] != '\"')
      {
        i++;
      }
      i++;
      cur = new_token(TK_STR, cur, p, i);
      p += i;
      continue;
    }

    if (*p == '\'')
    {
      cur = new_token(TK_NUM, cur, p, 0);
      p++;
      if (startswith(p, "\\0"))
      {
        cur->val = '\0';
        p += 3;
      }
      else if (startswith(p, "\\t"))
      {
        cur->val = '\t';
        p += 3;
      }
      else if (startswith(p, "\\n"))
      {
        cur->val = '\n';
        p += 3;
      }
      else
      {
        cur->val = *p;
        p += 2;
      }
      continue;
    }

    if (isdigit(*p))
    {
      cur = new_token(TK_NUM, cur, p, 0);
      char *q = p;
      cur->val = strtol(p, &p, 10);
      cur->len = p - q;
      continue;
    }

    if (strncmp(p, "return", 6) == 0 && !is_alnum(p[6]))
    {
      cur = new_token(TK_RETURN, cur, p, 6);
      p += 6;
      continue;
    }

    if (strncmp(p, "if", 2) == 0 && !is_alnum(p[2]))
    {
      cur = new_token(TK_IF, cur, p, 2);
      p += 2;
      continue;
    }

    if (strncmp(p, "else", 4) == 0 && !is_alnum(p[4]))
    {
      cur = new_token(TK_ELSE, cur, p, 4);
      p += 4;
      continue;
    }

    if (strncmp(p, "while", 5) == 0 && !is_alnum(p[5]))
    {
      cur = new_token(TK_WHILE, cur, p, 5);
      p += 5;
      continue;
    }

    if (strncmp(p, "for", 3) == 0 && !is_alnum(p[3]))
    {
      cur = new_token(TK_FOR, cur, p, 3);
      p += 3;
      continue;
    }

    if (strncmp(p, "int", 3) == 0 && !is_alnum(p[3]))
    {
      cur = new_token(TK_INT, cur, p, 3);
      p += 3;
      continue;
    }

    if (strncmp(p, "char", 4) == 0 && !is_alnum(p[4]))
    {
      cur = new_token(TK_CHAR, cur, p, 4);
      p += 4;
      continue;
    }

    if (strncmp(p, "sizeof", 6) == 0 && !is_alnum(p[6]))
    {
      cur = new_token(TK_SIZEOF, cur, p, 6);
      p += 6;
      continue;
    }

    if (strncmp(p, "struct", 6) == 0 && !is_alnum(p[6]))
    {
      cur = new_token(TK_STRUCT, cur, p, 6);
      p += 6;
      continue;
    }

    if (isalpha(*p))
    {
      char *q = p + 1;
      while (is_alnum(*q))
      {
        q++;
      }
      cur = new_token(TK_IDENT, cur, p, 0);
      cur->len = q - p;
      p = q;
      continue;
    }

    error_at(p, "トークナイズできません");
  }

  new_token(TK_EOF, cur, p, 0);
  return head.next;
}