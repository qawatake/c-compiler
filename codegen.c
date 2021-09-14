#include <stdio.h>
#include "9cc.h"

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

void gen_gvar()
{
  GVar *var = globals;
  while (var)
  {
    strprint(var->name, var->len);
    printf(":\n");
    printf("  .zero %d\n", size(var->type));
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
    cur =  cur->next;
  }
}

void gen_x86()
{
  printf(".intel_syntax noprefix\n");
  printf(".data\n");
  gen_str();
  gen_gvar();
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

  char *reg[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
  for (int i = 0; i < 6; i++)
  {
    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", 8 * (i + 1));
    printf("  mov [rax], %s\n", reg[i]);
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
    {
      gen(node->lhs);
    }
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
    switch (node->type->kind)
    {
    case TY_CHAR:
      printf("  movsx eax, byte ptr [rax]\n");
      break;
    case TY_INT_LITERAL:
    case TY_INT:
      printf("  mov eax, [rax]\n");
      break;
    case TY_PTR:
      printf("  mov rax, [rax]\n");
      break;
    }
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
    switch (node->type->kind)
    {
    case TY_CHAR:
      printf("  movsx eax, byte ptr [rax]\n");
      break;
    case TY_INT_LITERAL:
    case TY_INT:
      printf("  mov eax, [rax]\n");
      break;
    case TY_PTR:
      printf("  mov rax, [rax]\n");
      break;
    case TY_ARRAY: // 配列へのポインタ
      break;
    }

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

    switch (node->type->kind)
    {
    case TY_CHAR:
      printf("  mov [rax], dil\n");
      break;
    case TY_INT_LITERAL:
    case TY_INT:
      printf("  mov [rax], edi\n");
      break;
    case TY_PTR:
      printf("  mov [rax], rdi\n");
      break;
    }

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
  case ND_ADD:
    if (node->type->kind == TY_PTR)
    {
      // if (node->lhs->type->kind == TY_PTR && node->rhs->type->kind == TY_PTR)
      //   error("ポインタとポインタの演算が行われています");

      if (node->lhs->type->kind == TY_PTR || node->lhs->type->kind == TY_ARRAY)
      {
        gen(node->lhs);
        gen(node->rhs);
      }
      if (node->rhs->type->kind == TY_PTR || node->rhs->type->kind == TY_ARRAY)
      {
        gen(node->rhs);
        gen(node->lhs);
      }
      printf("  pop rax\n");
      printf("  mov rdi, %d\n", size(node->type->ptr_to));
      printf("  imul rax, rdi\n");
      printf("  push rax\n");
    }
    else
    {
      gen(node->lhs);
      gen(node->rhs);
    }
    printf("  pop rdi\n");
    printf("  pop rax\n");
    printf("  add rax, rdi\n");
    printf("  push rax\n");
    return;
  case ND_SUB:
    gen(node->lhs);
    gen(node->rhs);
    if (node->type->kind == TY_PTR)
    {
      if (node->rhs->type->kind == TY_PTR || node->rhs->type->kind == TY_ARRAY)
        error("引き算の左辺にポインタが配置されています");

      printf("  pop rax\n");
      printf("  mov rdi, %d\n", size(node->type->ptr_to));
      printf("  imul rax, rdi\n");
      printf("  push rax\n");
    }
    printf("  pop rdi\n");
    printf("  pop rax\n");
    printf("  sub rax, rdi\n");
    printf("  push rax\n");
    return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind)
  {
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