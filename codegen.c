#include <stdio.h>
#include "9cc.h"

static char *reg_str(RegKind kind, Size s)
{
  int size_id;
  switch (s)
  {
  case SIZE_CHAR:
    size_id = 2;
    break;
  case SIZE_INT:
    size_id = 1;
    break;
  case SIZE_PTR:
    size_id = 0;
    break;
  default:
    error("不適切なレジスタを使用しようとしています");
  }

  char *regs[][3] = {
      {"rdi", "edi", "dil"},
      {"rsi", "esi", "sil"},
      {"rdx", "edx", "dl"},
      {"rcx", "ecx", "cl"},
      {"r8", "r8d", "r8b"},
      {"r9", "r9d", "r9b"},
      {"rax", "eax", "al"},
  };

  return regs[kind][size_id];
}

static void switch_reg(RegKind lkind, RegKind rkind)
{
  printf("  push %s\n", reg_str(lkind, SIZE_PTR));
  printf("  push %s\n", reg_str(rkind, SIZE_PTR));
  printf("  pop %s\n", reg_str(lkind, SIZE_PTR));
  printf("  pop %s\n", reg_str(rkind, SIZE_PTR));
}

static void memory_to_reg(RegKind reg_from, RegKind reg_to, Size s)
{
  switch (s)
  {
  case SIZE_CHAR:
    printf("  movsx %s, byte ptr [%s]\n", reg_str(reg_to, SIZE_PTR), reg_str(reg_from, SIZE_PTR));
    break;
  case SIZE_INT:
    printf("  mov %s, dword ptr [%s]\n", reg_str(reg_to, s), reg_str(reg_from, SIZE_PTR));
    break;
  case SIZE_PTR:
    printf("  mov %s, qword ptr [%s]\n", reg_str(reg_to, s), reg_str(reg_from, SIZE_PTR));
  }
}

void gen_lval(Node *node)
{
  switch (node->kind)
  {
  case ND_LVAR:
    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", node->offset);
    printf("  push rax\n");
    break;
  case ND_GVAR:
    printf("  lea rax, ");
    strprint(node->name, node->len);
    printf("[rip]\n");
    printf("  push rax\n");
    break;
  case ND_DEREF:
    gen(node->lhs);
    break;
  case ND_DOT:
    gen_lval(node->lhs);
    printf("  pop rax\n");
    printf("  add rax, %d\n", node->type->offset);
    printf("  push rax\n");
    break;
  default:
    error("代入の左辺値が変数あるいは*変数ではありません");
  }
}

void align()
{
  printf("  push r10\n");
  printf("  push r11\n");
  printf("  push r12\n");
  printf("  mov r10, rax\n"); // rax を r10 に退避
  printf("  mov r11, rdx\n"); // rds を r11 に退避

  // rsp を 16 で割った余りを r12 に格納
  printf("  mov rax, rsp\n");
  printf("  cqo\n");
  printf("  mov r12, 16\n");
  printf("  idiv r12\n");

  printf("  sub rsp, rdx\n");
  printf("  mov r12, rdx\n"); // シフトする数を r12 にロード (r12 は関数呼び出しの前後に変化しないように規約で決まっている)
  printf("  mov rdx, r11\n"); // rdx を復元
  printf("  mov rax, r10\n"); // rax を復元
}

void dealign()
{
  printf("  add rsp, r12\n"); // シフトした分を戻す
  printf("  pop r12\n");
  printf("  pop r11\n");
  printf("  pop r10\n");
}

void gen_call(Node *node)
{
  if (node->kind != ND_CALL)
  {
    error("関数ではありません");
  }

  char *reg[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
  int narg = 0;
  Node *arg = node->lhs;
  if (arg != NULL)
  {
    gen(arg);
    while (arg != NULL)
    {
      narg++;
      arg = arg->lhs;
    }
    for (int i = 1; i <= narg; i++)
    {
      printf("  pop %s\n", reg[narg - i]);
    }
  }

  align();
  printf("  mov al, 0\n"); // 浮動小数点数の引数の個数をALに入れておく
  printf("  call ");
  for (int i = 0; i < node->len; i++)
  {
    printf("%c", node->name[i]);
  }
  printf("\n");
  dealign();
  printf("  push rax\n");
}

void gen_gvars()
{
  GVar *var = globals;
  while (var)
  {
    strprint(var->name, var->len);
    printf(":\n");

    gen_gvar(var->type, var->ini);
    var = var->next;
  }
}

void gen_str()
{
  String *cur = strings;
  while (cur)
  {
    printf(".LC%d:\n", cur->serial);
    printf("  .string ");
    strprint(cur->body, cur->len);
    printf("\n");
    cur = cur->next;
  }
}

void gen_x86()
{
  printf(".intel_syntax noprefix\n");
  printf(".data\n");
  gen_str();
  gen_gvars();
  Function *cur = funcs; // funcs は連結リストで実装されたスタックなので, 後で登録された関数が先にコード生成される
  printf(".text\n");
  while (cur)
  {
    gen_func(cur);
    cur = cur->next;
  }
}

void gen_func(Function *fn)
{
  printf(".globl %s\n", fn->name);
  printf("%s:\n", fn->name);

  // プロローグ
  printf("  push r12\n");
  printf("  push r13\n");
  printf("  push r14\n");
  printf("  push r15\n");
  printf("  push rbx\n");
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  printf("  sub rsp, %d\n", fn->offset);

  LVar *arg = fn->arg;
  int id = 0;
  while (arg)
  {
    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", arg->offset);
    printf("  mov [rax], %s\n", reg_str(id, size(arg->type)));
    id++;
    arg = arg->next;
  }

  gen(fn->body);
  printf("  pop rax\n");

  // エピローグ
  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  pop rbx\n");
  printf("  pop r15\n");
  printf("  pop r14\n");
  printf("  pop r13\n");
  printf("  pop r12\n");
  printf("  ret\n");
}

void gen(Node *node)
{
  switch (node->kind)
  {
  case ND_FUNC:
    return;
  case ND_NONE:
    printf("  push rax\n");
    return;
  case ND_COMP_STMT:
    gen(node->lhs);
    printf("  pop rax\n");
    gen(node->rhs);
    return;
  case ND_EXPR_STMT: // つながった expr の数だけスタックに積み上がる
    gen(node->rhs);
    if (node->lhs != NULL)
      gen(node->lhs);
    return;
  case ND_NUM:
    printf("  push %d\n", node->val);
    return;
  case ND_GVAR:
  case ND_LVAR:
    gen_lval(node);
    if (node->type->kind == TY_ARRAY)
      return;

    printf("  pop rax\n");
    memory_to_reg(REG_RAX, REG_RAX, size(node->type));
    printf("  push rax\n");
    return;
  case ND_DOT:
    gen_lval(node);
    printf("  pop rax\n");
    memory_to_reg(REG_RAX, REG_RAX, size(node->type));
    printf("  push rax\n");
    return;
  case ND_STR:
    printf("  lea rax, .LC%d[rip]\n", node->serial);
    printf("  push rax\n");
    return;
  case ND_ADDR:
    gen_lval(node->lhs);
    return;
  case ND_DEREF:
    gen(node->lhs);
    printf("  pop rax\n");
    if (node->type->kind != TY_ARRAY) // *(配列へのポインタ) が指すアドレス = 配列へのポインタが指すアドレス
      memory_to_reg(REG_RAX, REG_RAX, size(node->type));
    printf("  push rax\n");
    return;
  case ND_CALL:
    gen_call(node);
    return;
  case ND_ASSIGN:
    gen_lval(node->lhs);
    gen(node->rhs);
    printf("  pop rdi\n");
    printf("  pop rax\n");
    printf("  mov [rax], %s\n", reg_str(REG_RDI, size(node->type)));
    printf("  push rdi\n");
    return;
  case ND_IF:
    gen(node->lhs);
    if (node->rhs->kind == ND_ELSE)
    {
      printf("  pop rax;\n");
      printf("  cmp rax, 0\n");
      printf("  je .Lelse%x\n", lnum);
      gen(node->rhs->lhs);
      printf("  jmp .Lend%x\n", lnum);
      printf(".Lelse%x:\n", lnum);
      gen(node->rhs->rhs);
      printf(".Lend%x:\n", lnum);
      lnum++;
    }
    else
    {
      printf("  pop rax;\n");
      printf("  cmp rax, 0\n");
      printf("  je .Lend%x\n", lnum);
      gen(node->rhs);
      printf(".Lend%x:\n", lnum);
      printf("  push rax\n"); // スタックに1つ残すため
      lnum++;
    }
    return;
  case ND_WHILE:
    printf(".Lbegin%x:\n", lnum);
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je .Lend%x\n", lnum);
    gen(node->rhs);
    printf("  jmp .Lbegin%x\n", lnum);
    printf(".Lend%x:\n", lnum);
    printf("  push 0;"); // スタックに1つ残すため
    return;
  case ND_FOR:
    gen(node->lhs); // 初期化
    printf("  pop rax\n");
    gen(node->rhs); // while 文
    return;
  case ND_RETURN:
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  pop rbx\n");
    printf("  pop r15\n");
    printf("  pop r14\n");
    printf("  pop r13\n");
    printf("  pop r12\n");
    printf("  ret\n");
    return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind)
  {
  case ND_ADD:
    if (node->type->kind == TY_PTR)
    {
      if (node->rhs->type->kind == TY_PTR || node->rhs->type->kind == TY_ARRAY)
        switch_reg(REG_RAX, REG_RDI);
      printf("  mov rsi, %d\n", size(node->type->ptr_to));
      printf("  imul rdi, rsi\n");
    }
    printf("  add rax, rdi\n");
    break;
  case ND_SUB:
    if (node->type->kind == TY_PTR)
    {
      printf("  mov rsi, %d\n", size(node->type->ptr_to));
      printf("  imul rdi, rsi\n");
    }
    printf("  sub rax, rdi\n");
    break;
  case ND_MUL:
    printf("  imul rax, rdi\n");
    break;
  case ND_DIV:
    printf("  cqo\n");
    printf("  idiv rdi\n");
    break;
  case ND_EQ:
    printf("  cmp rax, rdi\n");
    printf("  sete al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_NE:
    printf("  cmp rax, rdi\n");
    printf("  setne al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_L:
    printf("  cmp rax, rdi\n");
    printf("  setl al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LE:
    printf("  cmp rax, rdi\n");
    printf("  setle al\n");
    printf("  movzb rax, al\n");
  }

  printf("  push rax\n");
}