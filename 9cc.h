#ifndef _INCLUDE_9CC_H_
#define _INCLUDE_9CC_H_
#include <stdbool.h>

// トークンの種類
typedef enum
{
  TK_RESERVED, // 記号
  TK_RETURN,   // return
  TK_IF,       // if
  TK_ELSE,     // else
  TK_WHILE,    // while
  TK_FOR,      // for
  TK_IDENT,    // 識別子
  TK_NUM,      // 整数トークン
  TK_EOF,      // 入力の終わりを表すトークン
} TokenKind;

// トークン型
typedef struct Token Token; // Token_tag Token
struct Token
{
  TokenKind kind; // トークンの型
  Token *next;    // 次の入力のトークン
  int val;        // kind が TK_NUM の場合, その数値
  char *str;      // トークン文字列
  int len;        // トークンの長さ 整数, EOFの場合は0
};

typedef struct LVar LVar;
// ローカル変数
struct LVar
{
  LVar *next; // 次の変数か NULL
  char *name; // 変数の名前
  int len;    // 変数の長さ
  int offset; // ベースポインタ (RBP) からのオフセット
};

// エラーを報告するための関数
// printfと同じ引数を取る
void error(char *fmt, ...);

// エラー箇所をを報告する
void error_at(char *loc, char *fmt, ...);

// 次のトークンが期待している記号のときには, トークンを1つ進めて真を返す
// それ以外の場合には偽を返す
bool consume(char *op);

// 次のトークンが識別子のときには, トークンを1つ進めてトークンを返す
// それ以外の場合には偽を返す
Token *consume_ident();

// 次のトークンが期待している記号のときには, トークンを1つ進める
// それ以外の場合にはエラーを報告する
void expect(char *op);

// 次のトークンが数値の場合, トークンを1つ読み進めてその数値を返す
// それ以外の場合にはエラーを報告する
int expect_number();

bool at_eof();

// 新しいトークンを作成して cur に繋げる
Token *new_token(TokenKind kind, Token *cur, char *str, int len);

// Multi-letter punctuator か判定
bool startswith(char *p, char *q);

// 英数字かアンダースコアかどうかを判定
bool is_alnum(char c);

// 変数を名前で検索する
// 見つからなかった場合はNULLを返す
LVar *find_lvar(Token *tok);

// 入力文字列 p をトークナイズしてそれを返す
Token *tokenize(char *p);

// 抽象構文木のノードの種類
typedef enum
{
  ND_ADD,       // +
  ND_SUB,       // -
  ND_MUL,       // *
  ND_DIV,       // /
  ND_EQ,        // ==
  ND_NE,        // !=
  ND_L,         // <
  ND_LE,        // <=
  ND_ASSIGN,    // =
  ND_NONE,      // None
  ND_COMP_STMT, // Compound Statement
  ND_RETURN,    // return
  ND_IF,        // if
  ND_ELSE,      // else
  ND_WHILE,     // while
  ND_FOR,       // for
  ND_LVAR,      // ローカル変数
  ND_NUM,       // 整数
} NodeKind;

typedef struct Node Node; // Node_tag Node

// 抽象構文木のノードの型
struct Node
{
  NodeKind kind; // ノードの型
  Node *lhs;     // 左辺
  Node *rhs;     // 右辺
  int val;       // kind が ND_NUM の場合のみ使う
  int offset;    // kind が ND_LVAR の場合のみ使う
};

Node *new_node(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);
void *program();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

void gen_lval(Node *node);
void gen(Node *node);

// 現在注目しているトークン
extern Token *token;

// ローカル変数
extern LVar *locals;

// 入力プログラム
extern char *user_input;

// 複数の式
extern Node *code[100];

// ループの数
extern int lnum;

#endif