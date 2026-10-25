#include "../include/vm.h"
#include "../include/eval.h"
#include "../include/parser.h"

#include <stdlib.h>

void initVM(ZynkVM *vm) {
	vm->exit_code = ZYNK_OK;
	vm->code = NULL;
	vm->sp = 0;
	vm->ip = 0;
	vm->ctx = newShellCtx();
	vm->exit_now = false;
	vm->constants = NULL;
	vm->n_cons = 0;
}

void freeVM(ZynkVM *vm) {
	if (vm->code==NULL || vm->ctx==NULL) return;
	killShellCtx(vm->ctx);
	for (int i=0;i<vm->n_cons;i++) {
		freeVal(vm->constants[i]);
	}
	free(vm->constants);
	free(vm->code);
	free(vm);
}

ShellValue stackPop(ZynkVM *vm) {
	if (vm->sp==0) return newNumVal(-1); // esto no debería pasar
	return vm->stack[vm->sp--];
}

void stackPush(ShellValue val, ZynkVM *vm) {
	if (vm->sp>=STACK_SIZE) {
		vm->exit_now = true;
		vm->exit_code = ZYNK_ERR;
		printf("Err -> Stack Max Size Exceeded!\n");
		return;
	}
	vm->stack[vm->sp++] = val;
}

static inline uint8_t getOP(ZynkVM *vm) {
	return vm->code[vm->ip++];
}
static inline uint16_t getWOP(ZynkVM *vm) {
	return getOP(vm) | getOP(vm) << 8;
}
static inline uint32_t getDOP(ZynkVM *vm) {
	return getWOP(vm) | getWOP(vm) << 16;
}
static inline uint64_t getQOP(ZynkVM *vm) {
	return getDOP(vm) | getDOP(vm) << 32;
}
static inline uint64_t getVAL(ZynkVM *vm) {
	return getQOP(vm) | getQOP(vm) << 64;
}

InterpretResult run(ZynkVM *vm) {
	uint8_t op = getOP(vm);
	while (1) {
		uint64_t arg
		ShellValue a;
		ShellValue b;
		switch (op) {
			case OP_HALT:
				vm->exit_now = true;
				break;
			case OP_LOAD_CONSTANT:
				stackPush(vm->constants[getOP(vm)], vm);
				break;
			case OP_LOAD_W_CONSTANT:
				stackPush(vm->constants[getWOP(vm)], vm);
				break;
			case OP_ADD:
				a = stackPop(vm);
				b = stackPop(vm);
				break;
			case OP_MUL:
				a = stackPop(vm);
				b = stackPop(vm);
				break;
			case OP_SUB:
				a = stackPop(vm);
				b = stackPop(vm);
				break;
			case OP_DIV:
				a = stackPop(vm);
				b = stackPop(vm);
				break;
			case OP_PUSH:
				break;
			case OP_POP:
				break;
			case OP_JUMP:
				break;
			case OP_JUMP_T:
				break;
			case OP_CALL:
				break;
			case OP_GET_MEMBER_CLOSURE:
				break;
			case OP_SET:
				break;
			case OP_GET:
				break;
			case OP_DEL:
				delShellVarEntry(vm->ctx, stackPop(vm).as.name);
				break;
			case OP_RET:
				break;
			case OP_EQ:
				break;
			case OP_NEQ:
				break;
			case OP_GT:
				break;
			case OP_LT:
				break;
			case OP_LEQ:
				break;
			case OP_GEQ:
				break;
			case OP_JUMP_F:
				break;
			case OP_NOT:
				break;
			case OP_OR:
				break;
			case OP_XOR:
				break;
			case OP_AND:
				break;
			case OP_ECHO:
				break;
		}
		if (vm->exit_now) break;
		op = getOP(vm);
	}
	return vm->exit_code;
}
