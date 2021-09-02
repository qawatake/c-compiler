#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "9cc.h"

// エラー箇所をを報告する
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

// 次のトークンが期待している記号のときには, トークンを1つ進めて真を返す
// それ以外の場合には偽を返す
bool consume(char *op)
{
  if (token->kind != TK_RESERVED && token->kind != TK_RETURN && token->kind != TK_IF && token->kind != TK_ELSE && token->kind != TK_WHILE && token->kind != TK_FOR)
    return false;
  else if (token->len != strlen(op) || memcmp(token->str, op, token->len))
    return false;
  token = token->next;
  return true;
}

// 次のトークンが識別子のときには, トークンを1つ進めてトークンを返す
// それ以外の場合には偽を返す
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

// 次のトークンが期待している記号のときには, トークンを1つ進める
// それ以外の場合にはエラーを報告する
void expect(char *op)
{
  if (token->kind != TK_RESERVED || token->len != strlen(op) || memcmp(token->str, op, token->len))
    error_at(token->str, "'%s'ではありません", op);
  token = token->next;
}

// 次のトークンが数値の場合, トークンを1つ読み進めてその数値を返す
// それ以外の場合にはエラーを報告する
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

// 変数を名前で検索する
// 見つからなかった場合はNULLを返す
LVar *find_lvar(Token *tok)
{
  for (LVar *var = locals; var; var = var->next)
  {
    if (var->len == tok->len && !memcmp(tok->str, var->name, var->len))
    {
      return var;
    }
  }
  return NULL;
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

    // Multi-letter punctuator
    if (startswith(p, "==") || startswith(p, "!=") || startswith(p, ">=") || startswith(p, "<="))
    {
      cur = new_token(TK_RESERVED, cur, p, 2);
      p += 2;
      continue;
    }

    // Single-letter punctuator
    if (strchr("+-*/()<>=;{},", *p))
    {
      cur = new_token(TK_RESERVED, cur, p++, 1);
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
  return node;
}

Node *parse_for_contents()
{
  Node *node = calloc(1, sizeof(Node));
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

void *program()
{
  locals = calloc(1, sizeof(LVar));
  locals->next = NULL;
  int i = 0;
  while (!at_eof())
  {
    code[i++] = function();
  }
  code[i] = NULL;
}

Node *function()
{
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_FUNC;

  Token *tok = consume_ident();
  if (!tok)
  {
    error("関数名ではありません");
  }
  node->name = tok->str;
  node->len = tok->len;

  expect("(");
  expect(")");
  expect("{");

  if (consume("}"))
  {
    node->lhs = new_node(ND_NONE, NULL, NULL);
    return node;
  }
  node->lhs = new_node(ND_COMP_STMT, stmt(), NULL);
  Node *cur = node->lhs;
  while (!consume("}"))
  {
    cur->rhs = new_node(ND_COMP_STMT, stmt(), NULL);
    cur = cur->rhs;
  }
  cur->rhs = new_node(ND_NONE, NULL, NULL);
  return node;
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
      node = new_node(ND_ASSIGN, node, assign());
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
    }
    else if (consume("!="))
    {
      node = new_node(ND_NE, node, relational());
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
    }
    else if (consume("<"))
    {
      node = new_node(ND_L, node, add());
    }
    else if (consume(">="))
    {
      node = new_node(ND_LE, add(), node);
    }
    else if (consume("<="))
    {
      node = new_node(ND_LE, node, add());
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
      node = new_node(ND_ADD, node, mul());
    }
    else if (consume("-"))
    {
      node = new_node(ND_SUB, node, mul());
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
      node = new_node(ND_MUL, node, unary());
    }
    else if (consume("/"))
    {
      node = new_node(ND_DIV, node, unary());
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
    return new_node(ND_SUB, new_node_num(0), primary());
  }
  if (consume("+"))
  {
    return primary();
  }
  return primary();
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
      node->kind = ND_CALL;
      node->lhs = parse_func_args();
      LVar *lvar = find_lvar(tok);
      if (lvar)
      {
        node->name = lvar->name;
        node->len = lvar->len;
      }
      else
      {
        lvar = calloc(1, sizeof(LVar));
        lvar->name = tok->str;
        lvar->len = tok->len;
        lvar->next = locals;
        locals = lvar;
        node->name = lvar->name;
        node->len = lvar->len;
      }
      return node;
    }

    node->kind = ND_LVAR;
    LVar *lvar = find_lvar(tok);
    if (lvar)
    {
      node->offset = lvar->offset;
    }
    else
    {
      lvar = calloc(1, sizeof(LVar));
      lvar->next = locals;
      lvar->name = tok->str;
      lvar->len = tok->len;
      lvar->offset = locals->offset + 8;
      node->offset = lvar->offset;
      locals = lvar;
    }
    return node;
  }

  // そうでなければ数値のはず
  return new_node_num(expect_number());
}