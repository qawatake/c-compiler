#ifndef _INCLUDE_9CC_H_
#define _INCLUDE_9CC_H_
#include <stdbool.h>

/* 変数・型の宣言 */

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
  TK_CHAR,     // char
  TK_SIZEOF,   // sizeof
  TK_STRUCT,   // struct
  TK_TYPEDEF,  // typedef
  TK_VOID,     // void
  TK_IDENT,    // 識別子
  TK_NUM,      // 整数トークン
  TK_STR,      // 文字列トークン
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

// 文字列型
typedef struct String String;
struct String
{
  String *next; // 次の文字列
  char *body;   // 文字列本体 (入力文字列へのポインタ)
  int len;      // 文字列の長さ (""を含む)
  int serial;   // 識別番号
};

// 変数のサイズ
typedef enum
{
  SIZE_CHAR = 1,
  SIZE_INT = 4,
  SIZE_PTR = 8,
} Size;

// 型の種類
typedef enum
{
  TY_INT_LITERAL, // 整数リテラル
  TY_CHAR,        // char
  TY_INT,         // int
  TY_PTR,         // ポインタ
  TY_ARRAY,       // 配列
  TY_STRUCT,      // struct
  TY_VOID,        // void
} TypeKind;

// 変数の型
typedef struct Type Type;
struct Type
{
  TypeKind kind;  // int or pointer
  Type *ptr_to;   // ~ 型へのポインタ
  int array_size; // 配列の要素数 (未設定の場合は -1)
  Type **members; // kind が TY_STRUCT の場合に使う. メンバ要素の可変長配列
  int offset;     // 構造体のメンバの場合に使う. 構造体内のオフセット
  char *name;     // 構造体のメンバの場合に使う
  int len;        // 構造体のメンバの場合に使う
};

// 変数の共通要素
typedef struct Var Var;
struct Var
{
  char *name; // 変数名
  int len;    // 変数名の長さ
  Type *type; // 変数の型
};

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
  ND_DOT,       // x.a
  ND_FUNC,      // 関数定義
  ND_LVAR,      // ローカル変数
  ND_GVAR,      // グローバル変数
  ND_NUM,       // 整数
  ND_STR,       // 文字列
  ND_ARRAY,     // 配列 {a, b, c}
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
  int serial;    // kind が ND_STR の場合使う
  Node **elems;  // kind が ND_ARRAY の場合使う. 配列要素の可変長配列
};

// ローカル変数
typedef struct LVar LVar;
struct LVar
{
  LVar *next; // 次の変数か NULL
  char *name; // 変数の名前
  int len;    // 変数の長さ
  int offset; // ベースポインタ (RBP) からのオフセット
  Type *type; // 変数の型
};

// グローバル変数
typedef struct GVar GVar;
struct GVar
{
  GVar *next; // 次の変数か NULL
  char *name; // 変数名
  int len;    // 変数名の長さ
  Type *type; // 変数の型
  Node *ini;  // 初期値
};

typedef struct Function Function;
struct Function
{
  Function *next; // 登録済みの次の関数
  char *name;
  Node *body;
  Type *retype; // 返り値の型
  int offset;   // ローカル変数のデータ保持のために必要なメモリ数
  LVar *arg;    // 引数の連結リスト
};

typedef struct Tag Tag;
struct Tag
{
  Tag *next; // 登録済みの次の構造体タグ
  char *name;
  int len;
  Type *type; // タグと紐付けられた型
};

typedef struct Tydef Tydef;
struct Tydef
{
  Tydef *next; // 登録済みの次の typedef
  char *name;
  int len;
  Type *type; // タグと紐付けられた型
};

typedef struct Scope Scope;
struct Scope
{
  Scope *parent;
  LVar *locals;
  Tag *tags;
  Tydef *typedefs;
  int offset; // 子スコープで使用された最大のメモリ数
};

typedef enum
{
  REG_RDI,
  REG_RSI,
  REG_RDX,
  REG_RCX,
  REG_R8,
  REG_R9,
  REG_RAX,
} RegKind;

/* グローバル変数の宣言 */

// 現在注目しているトークン
extern Token *token;

// 入力プログラム
extern char *user_input;

// ループの数
extern int lnum;

// スコープ
extern Scope *scope;

// 関数の連結リスト
extern Function *funcs;

// グローバル変数
extern GVar *globals;

// 複数の文字列
extern String *strings;

/* util.c */

// エラーを報告するための関数
// printfと同じ引数を取る
void error(char *fmt, ...);

// エラー箇所をを報告する
void error_at(char *loc, char *fmt, ...);

// コピー文字列を生成し, 先頭へのポインタを返す
char *duplicate(char *str, size_t len);

// ポインタの先から指定した文字列分表示
void strprint(char *str, size_t len);

// スコープの深さを返す
int scpdepth(Scope *scp);

// 構文木を表示する
void syntax_tree(int depth, Node *node);

// タイプの派生をたどる
void type_tree(Type *ty);

/* token.c
  トークナイザの実装
*/

// 次のトークンが期待している記号のときには, トークンを1つ進めて真を返す
// それ以外の場合には偽を返す
bool consume_reserve(char *op);

// 次のトークンが期待している種類のトークンのときには, トークンを1つ進めて真を返す
// それ以外の場合には偽を返す
bool consume(TokenKind kind);

// 次のトークンが識別子のときには, トークンを1つ進めてトークンを返す
// それ以外の場合には NULL を返す
Token *consume_ident();

// 次のトークンが文字列のときには, トークンを1つ進めてトークンを返す
// それ以外の場合には NULL を返す
String *consume_str();

// 次のトークンが数値の場合, トークンを1つ進めて, 真を返す
// 同時にその数値を参照渡しで返す
// それ以外の場合には偽を返す
bool consume_num(int *num);

// 次のトークンが期待している記号であれば, 真を返す
// それ以外の場合には偽を返す
// いずれの場合もトークンは進めない
bool check_reserve(char *op);

// 次のトークンが期待している記号であれば, 真を返す
// それ以外の場合には偽を返す
// いずれの場合もトークンは進めない
bool check(TokenKind kind);

// 次のトークンが typedef された型であれば, トークンを1つ進めて, 該当する Tydef * を返す
// それ以外の場合には NULL を返す
Tydef *consume_typedefs();

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

// 入力文字列 p をトークナイズしてそれを返す
Token *tokenize(char *p);

/* typeprs.c
  型宣言を解析
*/

// 新しい Type を生成
Type *new_type(TypeKind kind);
Type *new_type_ptr(Type *ptr_to);
Type *new_type_array(Type *ptr_to, int array_size);

// 合成したノードの型を返す
Type *tyjoin(Type *lty, Type *rty);

// sizeof に相当する役割
Size size(Type *ty);

// 型宣言を解析
// root: 変数宣言の冒頭の型
// var: 変数宣言を解析した結果 (name, len, type) を埋め込む
// 型の解析に成功したら, true を返す
bool assr(Var *var);

/* parse.c
  構文木を生成
  再帰降下構文解析器の実装
*/

Node *new_node(NodeKind kind, Node *lhs, Node *rhs, Type *ty);
Node *new_node_num(int val);
void program();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *comp();
Node *primary();

/* tree_preprocess.c
  構文木のプリプロセッサ
  配列初期化の構文木を再構成する
  再帰的な処理が必要なため, パースと同時に行うことは難しい
*/

// シリアル番号から該当する文字列を検索する
String *find_str(int serial);

void preprocess(Node **pnode);

/* codegen.c
  構文木に沿って, コード生成
*/

// call 呼び出し前のアラインメント
// r10, r11, r12 レジスタを退避
void align();
// call 呼び出し後 rsp を align 呼び出し前の状態に戻す
// r10, r11, r12 レジスタを復元
void dealign();

// 配列をポインタとして使うためのコード生成
void gen_lval(Node *node);
void gen_call(Node *node);
void gen_x86();
void gen_gvars();
void gen_func(Function *fn);
void gen(Node *node);

/* glblcodegen.c
  グローバル変数の初期化式を解析
*/
// グローバル変数の初期化式を解析
// アセンブリでの変数宣言のうち, 内容部分 (インデントされた部分) を出力する
void gen_gvar(Type *ty, Node *ini);

/* global.c
  globals, scope, funcs といったグローバル変数へのインタフェースを提供
*/

// 追加系
// ローカル変数を追加
void add_locals(LVar *lvar);
// グローバル変数を追加
void add_globals(GVar *gvar);
// typedef を追加
void add_typedef(Tydef *tydef);
// 構造体タグを追加
void add_tag(Tag *tag);
// 文字列を追加
void add_str(String *str);
// 関数を追加
void add_func(Function *fn);
// 現在のスコープに登録されているローカル変数を関数の引数として登録する
void add_arg(Function *fn);

// 検索系
// ローカル変数を変数名で検索する
// 見つからなかった場合は NULL を返す
LVar *find_lvar(Token *tok);

// グローバル変数を変数名検索する
// 見つからなかった場合は NULL を返す
GVar *find_gvar(char *name, int len);

// 関数を名前で検索する
// 見つからなかった場合は NULL を返す
Function *find_func(Token *tok);

// 構造体タグを検索
// 見つからなかった場合は NULL を返す
Tag *find_tag(char *name, int len);

// serial を使って文字列を検索
// 見つからなかった場合は NULL を返す
String *find_str(int serial);

// これまでに登録された文字列の数を返す
int count_strings();

// スコープの移動
// 1つ下のスコープに入る
void zoom_in();
// 現在のスコープを抜ける
void zoom_out();

#endif