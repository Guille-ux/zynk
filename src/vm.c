#include "../include/vm.h"
#include "../include/eval.h"
#include "../include/parser.h"

#include <stdlib.h>

ZynkVM vm;
Chunk chk;

void initVM(ZynkVM *vm) {
  vm->exit_code = ZYNK_OK;
  vm->code = NULL;
  vm->cp = 0;
  vm->sp = 0;
  vm->ip = 0;
  vm->ctx = newShellCtx();
  vm->exit_now = false;
  vm->constants = NULL;
  vm->n_cons = 0;
}

void freeVM(ZynkVM *vm) {
  if (vm->code == NULL || vm->ctx == NULL)
    return;
  killShellCtx(vm->ctx);
  for (int i = 0; i < vm->n_cons; i++) {
    freeVal(vm->constants[i]);
  }
  free(vm->constants);
  free(vm->code);
  // free(vm);
}

CallVal callPop(ZynkVM *vm) {
  if (vm->cp == 0)
    return (CallVal){.caller = 0, .caller_ctx = NULL}; // esto no debería pasar
  return vm->call_stack[--vm->cp];
}

static EvalCtx *contextFromSelf(ShellValue value) {
  if (value.type != VAL_DICT || value.as.comodin == NULL)
    return NULL;

  InstanceObj *instance =
      (InstanceObj *)((size_t)value.as.comodin - sizeof(GCObj));

  return instance->ctx;
}

void callPush(ZynkVM *vm, CallVal val) {
  if (vm->cp >= CALL_STACK_SIZE) {
    vm->exit_now = true;
    vm->exit_code = ZYNK_ERR;
    printf("Err -> Max Recursion Limit Exceed!\n");
    return;
  }
  vm->call_stack[vm->cp++] = val;
}

ShellValue stackPop(ZynkVM *vm) {
  if (vm->sp == 0)
    return newNumVal(-1); // esto no debería pasar
  return vm->stack[--vm->sp];
}

void stackPush(ZynkVM *vm, ShellValue val) {
  if (vm->sp >= STACK_SIZE) {
    vm->exit_now = true;
    vm->exit_code = ZYNK_ERR;
    printf("Err -> Stack Max Size Exceeded!\n");
    return;
  }
  vm->stack[vm->sp++] = ref_add(val);
}

static inline uint8_t getOP(ZynkVM *vm) {

  if (vm->code == NULL || vm->ip >= chk.count) {
    vm->exit_now = true;
    vm->exit_code = ZYNK_ERR;
    return OP_HALT;
  }
  return vm->code[vm->ip++];
}

static inline uint16_t getWOP(ZynkVM *vm) {
  uint16_t val;
  memcpy(&val, &vm->code[vm->ip], sizeof(uint16_t));
  vm->ip += sizeof(uint16_t);
  return val;
}

static inline uint32_t getDOP(ZynkVM *vm) {
  uint32_t val;
  memcpy(&val, &vm->code[vm->ip], sizeof(uint32_t));
  vm->ip += sizeof(uint32_t);
  return val;
}

static inline uint64_t getVal(ZynkVM *vm) {
  uint64_t val;
  memcpy(&val, &vm->code[vm->ip], sizeof(uint64_t));
  vm->ip += sizeof(uint64_t);
  return val;
}

static void printVal(ShellValue val) {
  if (val.type == VAL_NUMBER) {
    kprintf("%f\n", val.as.num);
  } else if (val.type == VAL_STRING) {
    kprintf("%s\n", (const char *)val.as.str);
  } else if (val.type == VAL_DICT) {
    InstanceObj *inst = (InstanceObj *)((size_t)val.as.comodin - sizeof(GCObj));
    EvalCtx *rctx = inst->ctx;
    for (int i = 0; i < rctx->var_cap; i++) {
      if (rctx->var_table[i].name == NULL)
        continue;
      printf("Name: %s -> ", rctx->var_table[i].name);
      if (rctx->var_table[i].val.type == VAL_STRING) {
        printf("%s\n", rctx->var_table[i].val.as.str);
      } else if (rctx->var_table[i].val.type == VAL_NUMBER) {
        printf("%lf\n", rctx->var_table[i].val.as.num);
      }
    }
  } else {
    kprintf("Err -> Unknown type\n");
  }
}

int vm_cycles = 0;

InterpretResult run(ZynkVM *vm) {
  uint8_t op = getOP(vm);
  while (1) {
    vm_cycles = (vm_cycles + 1) % 100;
    if (vm_cycles == 0) {
      zynk_gc();
    }
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
      if (a.type == VAL_STRING || b.type == VAL_STRING) {
        char *left;
        char *right;
        char num_buff[512];
        if (a.type == b.type) {
          left = a.as.str;
          right = b.as.str;
        } else if (a.type == VAL_NUMBER) {
          right = b.as.str;
          snprintf(num_buff, 512, "%lf", a.as.num);
          left = num_buff;
        } else if (b.type == VAL_NUMBER) {
          left = a.as.str;
          snprintf(num_buff, 512, "%lf", b.as.num);
          right = num_buff;
        }
        char *concat = (char *)kmalloc(strlen(left) + strlen(right) + 1);
        if (concat == NULL) {
          kprintf("[FATAL ERROR]\n");
          break; // IDK
        }
        strcpy(concat, left);
        strcat(concat, right);

        stackPush(vm, newStrVal(concat));
        kfree(concat);
      } else if (a.type == VAL_NUMBER && b.type == VAL_NUMBER) {
        stackPush(vm, newNumVal(a.as.num + b.as.num));
      }
      break;
    case OP_MUL:
      b = stackPop(vm);
      a = stackPop(vm);
      if (a.type != VAL_NUMBER || b.type != VAL_NUMBER) {
        printf("Err -> Expected Numbers");
        break;
      }
      stackPush(vm, newNumVal(a.as.num * b.as.num));
      break;
    case OP_SUB:
      b = stackPop(vm);
      a = stackPop(vm);
      if (a.type != VAL_NUMBER || b.type != VAL_NUMBER) {
        printf("Err -> Expected Numbers");
        break;
      }
      stackPush(vm, newNumVal(a.as.num - b.as.num));
      break;
    case OP_DIV:
      b = stackPop(vm);
      if (b.as.num == 0) {
        printf("Err -> Division By Zero\n");
        break; // uf nos salvamos del
               // division by zero
               // luego vemos si
               // merece la pena sacarlo
               // qn sabe
      }
      a = stackPop(vm);
      stackPush(vm, newNumVal(a.as.num / b.as.num));
      break;
    case OP_PUSH_UINT:
      // yay, pushing vals into the thing
      arg = getVal(vm);
      memcpy(&d, &arg, sizeof(uint64_t));
      a.type = VAL_UINT;
      a.as.uint = arg;
      stackPush(vm, a);
      break;
    case OP_PUSH:
      // yay, pushing vals into the thing
      arg = getVal(vm);
      memcpy(&d, &arg, sizeof(double));
      stackPush(vm, newNumVal(d));
      break;
    case OP_POP: {
      c = stackPop(vm); /* valor */
      b = stackPop(vm); /* nombre */
      a = stackPop(vm); /* objeto/contexto */

      EvalCtx *ctx = contextFromSelf(a);

      if (ctx == NULL || b.type != VAL_STRING || b.as.str == NULL) {
        vm->exit_now = true;
        vm->exit_code = ZYNK_ERR;
        printf("Err -> Invalid assignment target\n");
        break;
      }

      setShellVarEntry(ctx, b.as.str, c);
      break;
    }
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
    case OP_MAKE_FUNC: {
      a = stackPop(vm);
      ShellValue func_obj = newVMFuncVal((size_t)a.as.uint, vm->ctx);
      stackPush(vm, func_obj);
      break;
    }
    case OP_CALL: {
      a = stackPop(vm); // nargs
      int argc = (int)a.as.uint;
      idk.caller = vm->ip;
      idk.caller_ctx = vm->ctx;
      callPush(vm, idk);

      EvalCtx *eval_idk = newShellCtx();

      for (size_t i = 0; i < argc; i++) {
        b = stackPop(vm);
        addShellArg(eval_idk, b);
      }

      ShellValue fn_val = stackPop(vm); // La función (VAL_FUNC o VAL_CFUNC)

      if (fn_val.type == VAL_CFUNC) {
        callPop(vm);
        ShellValue *arr = kmalloc(sizeof(ShellValue) * argc);
        for (int i = 0; i < argc; i++) {
          arr[i] = eval_idk->argv[i].val;
        }
        stackPush(vm, fn_val.as.c_func->func(argc, arr));
        kfree(arr);
      } else if (fn_val.type == VAL_FUNC) {
        ZFuncVMObj *func =
            (ZFuncVMObj *)((size_t)fn_val.as.comodin - sizeof(GCObj));

        // ¡EL PADRE ES EL ÁMBITO DONDE SE CREÓ LA FUNCIÓN!
        eval_idk->parent = func->closure_ctx;

        vm->ip = func->ip;
        vm->ctx = eval_idk;
      } else {
        // Fallback si por alguna razón vino un puntero UINT crudo
        eval_idk->parent = vm->ctx;
        vm->ip = fn_val.as.uint;
        vm->ctx = eval_idk;
      }
      break;
    }
    case OP_GET_CLOSURE: {
      arg = getVal(vm);

      if (arg == 0 || arg > vm->sp || vm->ctx == NULL) {
        vm->exit_now = true;
        vm->exit_code = ZYNK_ERR;
        printf("Err -> Invalid member path\n");
        break;
      }

      EvalCtx *ctx = vm->ctx;
      bool valid = true;

      for (uint64_t i = 0; i < arg; i++) {
        b = stackPop(vm);

        if (b.type != VAL_STRING || b.as.str == NULL || ctx == NULL) {
          valid = false;
          break;
        }

        Var *base = getShellVarEntrySilent(ctx, b.as.str);

        if (base == NULL) {
          valid = false;
          break;
        }

        a = base->val;

        /*
         * Si quedan miembros, el valor actual debe ser
         * un objeto que contenga otro EvalCtx.
         */
        if (i + 1 < arg) {
          ctx = contextFromSelf(a);

          if (ctx == NULL) {
            valid = false;
            break;
          }
        }
      }

      if (!valid) {
        vm->exit_now = true;
        vm->exit_code = ZYNK_ERR;
        printf("Err -> Invalid or undefined member\n");
        break;
      }

      stackPush(vm, a);
      break;
    }
    case OP_GET: {
      b = stackPop(vm); /* nombre */
      a = stackPop(vm); /* objeto/contexto */

      EvalCtx *ctx = contextFromSelf(a);

      if (ctx == NULL || b.type != VAL_STRING || b.as.str == NULL) {
        vm->exit_now = true;
        vm->exit_code = ZYNK_ERR;
        printf("Err -> Invalid member access\n");
        break;
      }

      Var *entry = getShellVarEntry(ctx, b.as.str);

      if (entry == NULL) {
        vm->exit_now = true;
        vm->exit_code = ZYNK_ERR;
        break;
      }

      stackPush(vm, entry->val);
      break;
    }
    case OP_DEF:
      c = stackPop(vm); // value
      b = stackPop(vm); // name
      a = stackPop(vm); // closure
      eval_idk = contextFromSelf(a);
      defShellVarEntry(eval_idk, b.as.str, c);
      break;
    case OP_GET_ARG:
      stackPush(vm, getShellArg(vm->ctx, getVal(vm)));
      break;
    case OP_DEL: {
      b = stackPop(vm); /* nombre */
      a = stackPop(vm); /* objeto/contexto */

      EvalCtx *ctx = contextFromSelf(a);

      if (ctx == NULL || b.type != VAL_STRING || b.as.str == NULL) {
        vm->exit_now = true;
        vm->exit_code = ZYNK_ERR;
        printf("Err -> Invalid deletion target\n");
        break;
      }

      if (getShellVarEntrySilent(ctx, b.as.str) == NULL) {
        vm->exit_now = true;
        vm->exit_code = ZYNK_ERR;
        printf("Err -> Undefined variable\n");
        break;
      }

      delShellVarEntry(ctx, b.as.str);
      break;
    }
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
      if (vm->code[vm->ip - 1] == OP_PSELF) {
        // nothing
        // en plan las clases van
        // a devolver al final con esto
      } else {
        midKillShellCtx(eval_idk);
      }
      break;
    case OP_EQ:
      b = stackPop(vm);
      a = stackPop(vm);
      if (a.type != b.type) {
        stackPush(vm, newNumVal(0));
      } else {
        if (a.type == VAL_NUMBER) {
          if (a.as.num == b.as.num) {
            stackPush(vm, newNumVal(1));
          } else {
            stackPush(vm, newNumVal(0));
          }
        } else if (a.type == VAL_STRING) {
          if (strcmp(a.as.str, b.as.str) == 0) {
            stackPush(vm, newNumVal(1));
          } else {
            stackPush(vm, newNumVal(0));
          }
        } else {
          if (a.as.comodin == b.as.comodin) {
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
      if (a.type != b.type) {
        stackPush(vm, newNumVal(1));
      } else {
        if (a.type == VAL_NUMBER) {
          if (a.as.num == b.as.num) {
            stackPush(vm, newNumVal(0));
          } else {
            stackPush(vm, newNumVal(1));
          }
        } else if (a.type == VAL_STRING) {
          if (strcmp(a.as.str, b.as.str) == 0) {
            stackPush(vm, newNumVal(0));
          } else {
            stackPush(vm, newNumVal(1));
          }
        } else {
          if (a.as.comodin == b.as.comodin) {
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
      if (a.type != b.type) {
        stackPush(vm, newNumVal(0));
      } else if (a.type != VAL_NUMBER) {
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
      if (a.type != b.type) {
        stackPush(vm, newNumVal(0));
      } else if (a.type != VAL_NUMBER) {
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
      if (a.type != b.type) {
        stackPush(vm, newNumVal(0));
      } else if (a.type != VAL_NUMBER) {
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
      if (a.type != b.type) {
        stackPush(vm, newNumVal(0));
      } else if (a.type != VAL_NUMBER) {
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
      if (!isTruly(stackPop(vm))) {
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
    case OP_PSELF:
      a = newInstanceVal(vm->ctx);
      stackPush(vm, a);
      break;
    }
    if (vm->exit_now)
      break;
    op = getOP(vm);
  }
  return vm->exit_code;
}

void loadChunk(ZynkVM *vm, Chunk *chunk) {
  vm->code = chunk->code;
  vm->n_cons = chunk->constants_count;
  vm->constants = chunk->constants;
}
