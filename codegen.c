#include <stdio.h>
#include "9cc.h"

void gen_lval(Node *node)
{
  if (node->kind != ND_LVAR)
  {
    error("代入の左辺値が変数ではありません");
  }

  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", node->offset);
  printf("  push rax\n");
}

void gen_func(Node *node)
{
  if (node->kind != ND_FUNC)
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

  printf("  call ");
  for (int i = 0; i < node->len; i++)
  {
    printf("%c", node->name[i]);
  }
  printf("\n");
}

void gen(Node *node)
{
  switch (node->kind)
  {
  case ND_NONE:
    return;
  case ND_COMP_STMT:
    gen(node->lhs);
    printf("  pop rax\n");
    gen(node->rhs);
    printf("  pop rax\n");
    return;
  case ND_EXPR_STMT:
    gen(node->rhs);
    if (node->lhs != NULL)
    {
      gen(node->lhs);
    }
    return;
  case ND_NUM:
    printf("  push %d\n", node->val);
    return;
  case ND_LVAR:
    gen_lval(node);
    printf("  pop rax\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
    return;
  case ND_FUNC:
    gen_func(node);
    return;
  case ND_ASSIGN:
    gen_lval(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");
    printf("  mov [rax], rdi\n");
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
    return;
  case ND_FOR:
    gen(node->lhs);      // 初期化
    gen(node->rhs->rhs); // ブロック内処理 & 更新
    gen(node->rhs);      // while 文
  case ND_RETURN:
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
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
    printf("  add rax, rdi\n");
    break;
  case ND_SUB:
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