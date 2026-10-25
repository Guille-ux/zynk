#ifndef ZYNK_VM_H
#define ZYNK_VM_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "eval.h"

#define STACK_SIZE 256

typedef enum {
	ZYNK_OK,
	ZYNK_ERR,
} InterpretResult;

typedef struct {
	uint8_t *code;
	size_t sp;
	size_t ip;
	size_t n_cons;
	ShellValue *constants;
	ShellValue stack[STACK_SIZE];
	InterpretResult exit_code;
	EvalCtx *ctx; // si, bno es lo q hay no?
	bool exit_now;
} ZynkVM;

typedef enum {
	OP_HALT,
	OP_LOAD_CONSTANT,
	OP_LOAD_W_CONSTANT,
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_PUSH,
	OP_POP,
	OP_JUMP,
	OP_JUMP_T,
	OP_CALL,
	OP_GET_MEMBER_CLOSURE,
	OP_SET,
	OP_GET,
	OP_DEL,
	OP_RET,
	OP_EQ,
	OP_NEQ,
	OP_GT,
	OP_LT,
	OP_LEQ,
	OP_GEQ,
	OP_JUMP_F,
	OP_NOT,
	OP_OR,
	OP_XOR,
	OP_AND,
	OP_ECHO,
} OP;

void initVM(ZynkVM *vm);
void freeVM(ZynkVM *vm);
ShellValue stackPop(ZynkVM *vm);
void stackPush(ShellValue val, ZynkVM *vm);
InterpretResult run(ZynkVM *vm);

#endif
