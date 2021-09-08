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
  TK_INT,      // int
  TK_SIZEOF,   // sizeof
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

// 変数のサイズ
typedef enum
{
  SIZE_INT = 4,
  SIZE_PTR = 8,
} Size;

// 変数の型
typedef struct Type Type;
struct Type
{
  enum
  {
    TY_INT_LITERAL, // 整数リテラル
    TY_INT,         // int
    TY_PTR          // ポインタ
  } kind;           // int or pointer
  Type *ptr_to;     // ~ 型へのポインタ
};

Size size(Type *ty);

typedef struct LVar LVar;
// ローカル変数
struct LVar
{
  LVar *next; // 次の変数か NULL
  char *name; // 変数の名前
  int len;    // 変数の長さ
  int offset; // ベースポインタ (RBP) からのオフセット
  Type *type; // 変数の型
};

// エラーを報告するための関数
// printfと同じ引数を取る
void error(char *fmt, ...);

// エラー箇所をを報告する
void error_at(char *loc, char *fmt, ...);

char *duplicate(char *str, size_t len);

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
  ND_EXPR_STMT, // Expression Statement
  ND_CALL,      // 関数呼び出し
  ND_RETURN,    // return
  ND_IF,        // if
  ND_ELSE,      // else
  ND_WHILE,     // while
  ND_FOR,       // for
  ND_ADDR,      // &x
  ND_DEREF,     // *x
  ND_FUNC,      // 関数定義
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
  Type *type;    // kind が ND_LVAR の場合のみ使う
  char *name;    // kind が ND_CALL の場合のみ使う
  int len;       // kind が ND_CALL の場合のみ使う
};

typedef struct Function Function;
struct Function
{
  char *name;
  Node *body;
  Type *retype; // 返り値の型
};

// 関数を名前で検索する
// 見つからなかった場合はNULLを返す
Function *find_func(Token *tok);

// ノードの型を比較
// lty の方が強い型ならば, -1 を返す
// 同じ型ならば, 0 を返す
// rty の方が強い方ならば, 1 を返す
// 型の強さ: 2つの型を2項演算子で結んだときに, 結果の型になるような型を「強い」とする
// 型の強さの例: TY_INT_LITERAL < TY_INT < TY_INT
int tycmp(Type *lty, Type *rty);

// 合成したノードの型を返す
Type *tyjoin(Type *lty, Type *rty);

Node *new_node(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);
Node *parse_for_contents();
void program();
Function *function();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *parse_func_args();
Node *primary();

// call 呼び出し前のアラインメント
// r10, r11, r12 レジスタが書き換えられる
void align();
// call 呼び出し後のアラインメント
void dealign();

void gen_lval(Node *node);
void gen_call(Node *node);
void gen_func(Function *fn);
void gen(Node *node);

typedef struct Scope Scope;
struct Scope
{
  Scope *parent;
  LVar *locals;
};

// 1つ下のスコープに入る
void zoom_in();
// 現在のスコープを抜ける
void zoom_out();

// 現在注目しているトークン
extern Token *token;

// ローカル変数
extern LVar *locals;

// 入力プログラム
extern char *user_input;

// ループの数
extern int lnum;

// 複数の関数
extern Function *fns[100];

// スコープ
extern Scope *scope;

#endif