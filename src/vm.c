#include "../include/vm.h"
#include "../include/eval.h"
#include "../include/parser.h"

#include <stdlib.h>

ZynkVM vm;
Chunk chk;



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

CallVal callPop(ZynkVM *vm) {
	if (vm->cp==0) return (CallVal){.caller=0, .caller_ctx=NULL}; // esto no debería pasar
	return vm->call_stack[vm->cp--];
}

void callPush(ZynkVM *vm, CallVal val) {
	if (vm->cp>=CALL_STACK_SIZE) {
		vm->exit_now = true;
		vm->exit_code = ZYNK_ERR;
		printf("Err -> Max Recursion Limit Exceed!\n");
		return;
	}
	vm->call_stack[vm->cp++] = val;
}

ShellValue stackPop(ZynkVM *vm) {
	if (vm->sp==0) return newNumVal(-1); // esto no debería pasar
	return vm->stack[vm->sp--];
}

void stackPush(ZynkVM *vm, ShellValue val) {
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

static void printVal(ShellValue val) {
	if (val.type == VAL_NUMBER) {
		kprintf("%f\n", val.as.num);
	} else if (val.type == VAL_STRING) {
		kprintf("%s\n", (const char *)val.as.str);
	} else if (val.type == VAL_DICT) {
		InstanceObj *inst = (InstanceObj*)((size_t)val.as.comodin - sizeof(GCObj));
		EvalCtx *rctx = inst->ctx;	
		for (int i=0;i<rctx->var_cap;i++) {
			if (rctx->var_table[i].name==NULL) continue;
			printf("Name: %s -> ", rctx->var_table[i].name);
			if (rctx->var_table[i].val.type==VAL_STRING) {
				printf("%s\n", rctx->var_table[i].val.as.str);
			} else if (rctx->var_table[i].val.type==VAL_NUMBER){
				printf("%lf\n", rctx->var_table[i].val.as.num);
			}
		}
	} else {
		kprintf("Err -> Unknown type\n");
	}
}

InterpretResult run(ZynkVM *vm) {
	uint8_t op = getOP(vm);
	while (1) {
		uint64_t arg;
		ShellValue a;
		ShellValue b;
		ShellValue c; // i didn't expect to use more than
			      // 2 arguments idk
		EvalCtx *eval_idk;
		CallVal idk;
		double d;
		switch (op) {
			case OP_HALT:
				vm->exit_now = true;
				break;
			case OP_LOAD_CONSTANT:
				stackPush(vm, vm->constants[getOP(vm)]);
				break;
			case OP_LOAD_W_CONSTANT:
				stackPush(vm, vm->constants[getWOP(vm)]);
				break;
			case OP_ADD:
				b = stackPop(vm);
				a = stackPop(vm);
				if (a.type==VAL_STRING||b.type==VAL_STRING) {
					char *left;
					char *right;
					char num_buff[512];
					if (a.type==b.type) {
						left = a.as.str;
						right = b.as.str;
					} else if (a.type==VAL_NUMBER) {
						right = b.as.str;
						snprintf(num_buff, 512, "%lf", a.as.num);
						left = num_buff;
					} else if (b.type==VAL_NUMBER) {
						left = a.as.str;
						snprintf(num_buff, 512, "%lf", b.as.num);
						right = num_buff;
					}
				} else if (a.type==VAL_NUMBER && b.type==VAL_NUMBER) {
					stackPush(vm, newNumVal(a.as.num+b.as.num));
				}	
				break;
			case OP_MUL:
				b = stackPop(vm);
				a = stackPop(vm);
				if (a.type!=VAL_NUMBER || b.type!=VAL_NUMBER) {
					printf("Err -> Expected Numbers");
					break;
				}
				stackPush(vm, newNumVal(a.as.num*b.as.num));		
				break;
			case OP_SUB:
				b = stackPop(vm);
				a = stackPop(vm);
				if (a.type!=VAL_NUMBER || b.type!=VAL_NUMBER) {
					printf("Err -> Expected Numbers");
					break;
				}
				stackPush(vm, newNumVal(a.as.num-b.as.num));
				break;
			case OP_DIV:
				b = stackPop(vm);
				if (b.as.num==0) {
					printf("Err -> Division By Zero\n");
					break; // uf nos salvamos del
					       // division by zero
					       // luego vemos si
					       // merece la pena sacarlo
					       // qn sabe
				}
				a = stackPop(vm);
				stackPush(vm, newNumVal(a.as.num/b.as.num));
				break;
			case OP_PUSH:
				// yay, pushing vals into the thing
				arg = getVal(vm);
				memcpy(&d, &arg, sizeof(double));
				stackPush(vm, newNumVal(d));
				break;
			case OP_POP:
				// idk why i added this instruction
				// ahh yes, pop to a variable
				// first is the var
				// second is the value
				c = stackPop(vm); // value
				b = stackPop(vm); // name
				a = stackPop(vm); // closure

				setShellVarEntry((EvalCtx*)a.as.comodin, b.as.str, c);
				break;
			case OP_JUMP:
				arg = getVal(vm);
				vm->ip = (size_t)arg; // YAYAYAYAYA
				break;
			case OP_JUMP_T:
				a = stackPop(vm);
				arg = getVal(vm);
				if (isTruly(a)) {
					vm->ip = (size_t)arg;
				}
				break;
			case OP_CALL:
				// yeah we push the data
				// to be able to come back
				// some time yk
				a = stackPop(vm); // nargs
				idk.caller = vm->ip;
				idk.caller_ctx = vm->ctx;
				callPush(idk);
				EvalCtx *eval_idk = newShellCtx();
				for (size_t i=0;i<(int)a.as.num;i++) {
					// achus
					b = stackPop(vm);
					addShellArg(eval_idk, b);
				}
				a = stackPop(vm);
				vm->ip = a.as.uint; // yk, the func
				vm->ctx = eval_idk;
				break;
			case OP_GET_CLOSURE:
				arg = getVal(vm); // num of things idk
				a.type = VAL_DICT;
				a.as.comodin = vm->ctx; // yk, we start
							// where we 
							// are right now
				for (int i=0;i<(int)arg;i++) {
					b = stackPop(vm);
					Var *base = getShellVarEntrySilent((EvalCtx*)a.as.comodin, b.as.str);
					a = base->val;
				}
				stackPush(a);
				break;
			case OP_GET:
				// tomamos del closure encontrado antes
				// yk
				b = stackPop(vm); // el nombre yk
				a = stackPop(vm); // el closure
				b = getShellVarEntry((EvalCtx*)a.as.comodin, b.as.str);
				stackPush(vm, b); // now, time to push
				break;
			case OP_DEF:
				c = stackPop(vm); // value
				b = stackPop(vm); // name
				a = stackPop(vm); // closure

				defShellVarEntry((EvalCtx*)a.as.comodin, b.as.str, c);
				break;
			case OP_GET_ARG:
				stackPush(getShellArg(vm->ctx, getVal(vm)));
				break;
			case OP_DEL:
				b = stackPop(vm); // name
				a = stackPop(vm); // closure
				delShellVarEntry((EvalCtx*)a.as.comodin, b.as.str);
				break;
			case OP_RET:
				// yeah does nothing, well, returns
				idk = callPop(vm);
				arg = (uint64_t)idk.caller;
				// no se q cosa de volver, ahhhh
				// esto de aqui creo
				vm->ip = (size_t)(arg); // NICE, SALTO
				eval_idk = vm->ctx;
				vm->ctx = idk.caller_ctx; // jeje
							   // CLOSURES
				midKillShellCtx(eval_idk);
				break;
			case OP_EQ:
				b = stackPop(vm);
				a = stackPop(vm);
				if (a.type!=b.type) {
					stackPush(vm, newNumVal(0));
				} else {
					if (a.type==VAL_NUMBER) {
						if (a.as.num==b.as.num) {
							stackPush(vm, newNumVal(1));
						} else {
							stackPush(vm, newNumVal(0));
						}
					} else if (a.type==VAL_STRING) {
						if (strcmp(a.as.str, b.as.str)==0) {
							stackPush(vm, newNumVal(1));
						} else {
							stackPush(vm, newNumVal(0));
						}
					} else {
						if (a.as.comodin==b.as.comodin) {
							stackPush(vm, newNumVal(1));
						} else {
							stackPush(vm, newNumVal(0));
						}
					}
				}
				break;
			case OP_NEQ:
				b = stackPop(vm);
				a = stackPop(vm);
				if (a.type!=b.type) {
					stackPush(vm, newNumVal(1));
				} else {
					if (a.type==VAL_NUMBER) {
						if (a.as.num==b.as.num) {
							stackPush(vm, newNumVal(0));
						} else {
							stackPush(vm, newNumVal(1));
						}
					} else if (a.type==VAL_STRING) {
						if (strcmp(a.as.str, b.as.str)==0) {
							stackPush(vm, newNumVal(0));
						} else {
							stackPush(vm, newNumVal(1));
						}
					} else {
						if (a.as.comodin==b.as.comodin) {
							stackPush(vm, newNumVal(0));
						} else {
							stackPush(vm, newNumVal(1));
						}
					}
				}
				break;
			case OP_GT:
				b = stackPop(vm);
				a = stackPop(vm);
				if (a.type!=b.type) {
					stackPush(vm, newNumVal(0));
				} else  if (a.type!=VAL_NUMBER) {
					stackPush(vm, newNumVal(0));
				} else {
					if (a.as.num > b.as.num) {
						stackPush(vm, newNumVal(1));
					} else {
						stackPush(vm, newNumVal(0));
					}
				}
				break;
			case OP_LT:
				b = stackPop(vm);
				a = stackPop(vm);
				if (a.type!=b.type) {
					stackPush(vm, newNumVal(0));
				} else  if (a.type!=VAL_NUMBER) {
					stackPush(vm, newNumVal(0));
				} else {
					if (a.as.num < b.as.num) {
						stackPush(vm, newNumVal(1));
					} else {
						stackPush(vm, newNumVal(0));
					}
				}
				break;
			case OP_LEQ:
				b = stackPop(vm);
				a = stackPop(vm);
				if (a.type!=b.type) {
					stackPush(vm, newNumVal(0));
				} else  if (a.type!=VAL_NUMBER) {
					stackPush(vm, newNumVal(0));
				} else {
					if (a.as.num <= b.as.num) {
						stackPush(vm, newNumVal(1));
					} else {
						stackPush(vm, newNumVal(0));
					}
				}
				break;
			case OP_GEQ:
				b = stackPop(vm);
				a = stackPop(vm);
				if (a.type!=b.type) {
					stackPush(vm, newNumVal(0));
				} else  if (a.type!=VAL_NUMBER) {
					stackPush(vm, newNumVal(0));
				} else {
					if (a.as.num >= b.as.num) {
						stackPush(vm, newNumVal(1));
					} else {
						stackPush(vm, newNumVal(0));
					}
				}
				break;
			case OP_JUMP_F:
				a = stackPop(vm);
				arg = getVal(vm);
				if (!isTruly(a)) {
					vm->ip = (size_t)arg;
				}
				break;
			case OP_NOT:
				if (isTruly(stackPop(vm))) {
					stackPush(vm, newNumVal(1));
				} else {
					stackPush(vm, newNumVal(0));
				}
				break;
			case OP_OR:
				b = stackPop(vm);
				a = stackPop(vm);
				if (isTruly(a) || isTruly(b)) {
					stackPush(vm, newNumVal(1));
				} else {
					stackPush(vm, newNumVal(0));
				}
				break;
			case OP_XOR:
				b = stackPop(vm);
				a = stackPop(vm);
				if (isTruly(a) != isTruly(b)) {
					stackPush(vm, newNumVal(1));
				} else {
					stackPush(vm, newNumVal(0));
				}
				break;
			case OP_AND:
				b = stackPop(vm);
				a = stackPop(vm);
				if (isTruly(a) && isTruly(b)) {
					stackPush(vm, newNumVal(1));
				} else {
					stackPush(vm, newNumVal(0));
				}
				break;
			case OP_ECHO:
				printVal(stackPop(vm));
				break;
		}
		if (vm->exit_now) break;
		op = getOP(vm);
	}
	return vm->exit_code;
}

void loadChunk(ZynkVM *vm, Chunk *chunk) {
	vm->code = chunk->code;
	vm->n_cons = chunk->constants_count;
	vm->constants = chunk->constants;
}
