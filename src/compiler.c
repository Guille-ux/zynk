#include "../include/compiler.h"
#include "../include/chunk.h"
#include "../include/parser.h"
#include "../include/vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t disassembleInstruction(Chunk *chunk, size_t offset) {
  printf("%04zu ", offset);
  uint8_t instruction = chunk->code[offset];

  switch (instruction) {
  case OP_HALT:
    printf("OP_HALT\n");
    return offset + 1;
  case OP_ECHO:
    printf("OP_ECHO\n");
    return offset + 1;
  case OP_PUSH: {
    uint64_t val;
    memcpy(&val, &chunk->code[offset + 1], sizeof(uint64_t));
    double d;
    memcpy(&d, &val, sizeof(double));
    printf("OP_PUSH          %lf\n", d);
    return offset + 1 + 8;
  }
  case OP_PUSH_UINT: {
    uint64_t val;
    memcpy(&val, &chunk->code[offset + 1], sizeof(uint64_t));
    printf("OP_PUSH_UINT     %lu\n", val);
    return offset + 1 + 8;
  }
  case OP_LOAD_CONSTANT: {
    uint8_t constant_idx = chunk->code[offset + 1];
    printf("OP_LOAD_CONSTANT %d", constant_idx);
    if (chunk->constants != NULL) {
      printf(" (val type: %d)", chunk->constants[constant_idx].type);
    }
    printf("\n");
    return offset + 2;
  }
  case OP_LOAD_W_CONSTANT: {
    uint16_t constant_idx;
    memcpy(&constant_idx, &chunk->code[offset + 1], sizeof(uint16_t));
    printf("OP_LOAD_W_CONST  %u\n", (unsigned)constant_idx);
    return offset + 1 + sizeof(uint16_t);
  }
  case OP_GET_ARG: {
    uint64_t index;
    memcpy(&index, &chunk->code[offset + 1], sizeof(index));
    printf("OP_GET_ARG       %llu\n", (unsigned long long)index);
    return offset + 1 + sizeof(index);
  }
  case OP_GET_CLOSURE: {
    uint64_t depth;
    memcpy(&depth, &chunk->code[offset + 1], sizeof(depth));
    printf("OP_GET_CLOSURE   %llu\n", (unsigned long long)depth);
    return offset + 1 + sizeof(depth);
  }
  case OP_JUMP:
  case OP_JUMP_T:
  case OP_JUMP_F: {
    uint64_t target;
    memcpy(&target, &chunk->code[offset + 1], sizeof(uint64_t));
    const char *op_name = (instruction == OP_JUMP)     ? "OP_JUMP"
                          : (instruction == OP_JUMP_T) ? "OP_JUMP_T"
                                                       : "OP_JUMP_F";
    printf("%-16s target -> %lu\n", op_name, target);
    return offset + 1 + 8;
  }
  case OP_ADD:
    printf("OP_ADD\n");
    return offset + 1;
  case OP_SUB:
    printf("OP_SUB\n");
    return offset + 1;
  case OP_MUL:
    printf("OP_MUL\n");
    return offset + 1;
  case OP_DIV:
    printf("OP_DIV\n");
    return offset + 1;
  case OP_POP:
    printf("OP_POP\n");
    return offset + 1;
  case OP_CALL:
    printf("OP_CALL\n");
    return offset + 1;
  case OP_GET:
    printf("OP_GET\n");
    return offset + 1;
  case OP_DEF:
    printf("OP_DEF\n");
    return offset + 1;
  case OP_DEL:
    printf("OP_DEL\n");
    return offset + 1;
  case OP_RET:
    printf("OP_RET\n");
    return offset + 1;
  case OP_EQ:
    printf("OP_EQ\n");
    return offset + 1;
  case OP_NEQ:
    printf("OP_NEQ\n");
    return offset + 1;
  case OP_GT:
    printf("OP_GT\n");
    return offset + 1;
  case OP_LT:
    printf("OP_LT\n");
    return offset + 1;
  case OP_LEQ:
    printf("OP_LEQ\n");
    return offset + 1;
  case OP_GEQ:
    printf("OP_GEQ\n");
    return offset + 1;
  case OP_NOT:
    printf("OP_NOT\n");
    return offset + 1;
  case OP_OR:
    printf("OP_OR\n");
    return offset + 1;
  case OP_AND:
    printf("OP_AND\n");
    return offset + 1;
  case OP_PSELF:
    printf("OP_PSELF\n");
    return offset + 1;
  default:
    printf("UNKNOWN_OPCODE %d\n", instruction);
    return offset + 1;
  }
}

void disassembleChunk(Chunk *chunk, const char *name) {
  printf("=== MÁQUINA VIRTUAL ZYNK: %s ===\n", name);
  printf("Total bytes: %zu | Total constantes: %d\n", chunk->count,
         chunk->constants_count);

  size_t offset = 0;
  while (offset < chunk->count) {
    offset = disassembleInstruction(chunk, offset);
  }
  printf("========================================\n");
}

static void compileNode(ASTNode *node);
static void compileExpr(ASTNode *expr);

static void emitConstantIndex(int index) {
  if (index < 0)
    return;
  if (index <= 255) {
    writeChunk(&chk, OP_LOAD_CONSTANT);
    writeChunk(&chk, (uint8_t)index);
  } else {
    writeChunk(&chk, OP_LOAD_W_CONSTANT);
    writeChunk(&chk, (uint8_t)(index & 0xFF));
    writeChunk(&chk, (uint8_t)((index >> 8) & 0xFF));
  }
}

static void emitStringConstant(const char *text) {
  ShellValue value = newStrVal(text != NULL ? text : "");
  emitConstantIndex(addConstant(&chk, value));
}

static size_t countMembers(ASTNode *member) {
  size_t count = 0;
  while (member != NULL) {
    count++;
    member = member->data.member.member;
  }
  return count;
}

static ASTNode *lastMember(ASTNode *member) {
  if (member == NULL)
    return NULL;
  while (member->data.member.member != NULL)
    member = member->data.member.member;
  return member;
}

static void emitMemberNamesReverse(ASTNode *member, size_t limit) {
  if (member == NULL || limit == 0)
    return;

  emitMemberNamesReverse(member->data.member.member, limit - 1);
  compileExpr(member->data.member.expr);
}

static void emitMemberPath(const char *root, ASTNode *members,
                           size_t member_limit) {
  /*
   * La pila es LIFO.
   *
   * Para root.a.b emitimos:
   *     "b", "a", "root"
   *
   * El VM los extrae en este orden:
   *     "root", "a", "b"
   */
  emitMemberNamesReverse(members, member_limit);
  emitStringConstant(root);

  writeChunk(&chk, OP_GET_CLOSURE);
  writeChunkQ(&chk, (uint64_t)(member_limit + 1));
}

static void emitTarget(ASTNode *members, const char *root) {
  if (members == NULL) {
    /* Variable normal: root */
    writeChunk(&chk, OP_PSELF);
    emitStringConstant(root);
    return;
  }

  size_t count = countMembers(members);
  ASTNode *last = lastMember(members);

  if (count == 0 || last == NULL) {
    writeChunk(&chk, OP_PSELF);
    emitStringConstant(root);
    return;
  }

  /*
   * Para root.a.b = valor:
   * - Obtener root.a.
   * - Dejar "b" como nombre del destino.
   * - El compilador emitirá después el valor.
   */
  emitMemberPath(root, members, count - 1);
  compileExpr(last->data.member.expr);
}

typedef struct {
  size_t jumps[64];
  size_t count;
} LoopContext;

static LoopContext loop_stack[32];
static int loop_depth = -1;

static size_t emitJump(uint8_t op) {
  writeChunk(&chk, op);
  size_t jump_pos = chk.count;
  writeChunkQ(&chk, 0);
  return jump_pos;
}

static void patchJump(size_t jump_pos) {
  uint64_t target = chk.count;
  memcpy(&chk.code[jump_pos], &target, sizeof(uint64_t));
}

int compile(ASTNode *ast) {
  if (ast == NULL)
    return ZYNK_ERR;

  initChunk(&chk);
  loop_depth = -1;
  compileNode(ast);
  writeChunk(&chk, OP_HALT);

  return ZYNK_OK;
}

static void compileNode(ASTNode *node) {
  if (node == NULL)
    return;

  switch (node->type) {
  case NODE_PROGRAM: {
    for (size_t i = 0; i < node->data.program.size; i++) {
      compileNode(node->data.program.stmts[i]);
    }
    break;
  }
  case NODE_ECHO: {
    compileExpr(node->data.echo_stmt.expr);
    writeChunk(&chk, OP_ECHO);
    break;
  }
  case VAR_DEF: {
    emitTarget(node->data.var_def.member, node->data.var_def.var_name);
    compileExpr(node->data.var_def.expr);
    writeChunk(&chk, OP_DEF);
    break;
  }
  case VAR_DEL: {
    emitTarget(node->data.var_del.member, node->data.var_del.name);
    writeChunk(&chk, OP_DEL);
    break;
  }
  case NODE_ASSIGNMENT: {
    emitTarget(node->data.assignment.member, node->data.assignment.var_name);
    compileExpr(node->data.assignment.expr);
    writeChunk(&chk, OP_POP);
    break;
  }
  case NODE_IF: {
    compileExpr(node->data.if_stmt.condition);
    size_t jump_f_pos = emitJump(OP_JUMP_F);
    for (size_t i = 0; i < node->data.if_stmt.s_then; i++) {
      compileNode(node->data.if_stmt.thenb[i]);
    }
    if (node->data.if_stmt.elseb != NULL) {
      size_t jump_skip_else = emitJump(OP_JUMP);
      patchJump(jump_f_pos);
      for (size_t i = 0; i < node->data.if_stmt.s_else; i++) {
        compileNode(node->data.if_stmt.elseb[i]);
      }
      patchJump(jump_skip_else);
    } else {
      patchJump(jump_f_pos);
    }
    break;
  }
  case NODE_WHILE_LOOP: {
    size_t loop_start = chk.count;

    loop_depth++;
    loop_stack[loop_depth].count = 0;

    compileExpr(node->data.while_loop.condition);
    size_t jump_exit = emitJump(OP_JUMP_F);

    for (size_t i = 0; i < node->data.while_loop.s_body; i++) {
      compileNode(node->data.while_loop.body[i]);
    }

    size_t jump_back = emitJump(OP_JUMP);
    uint64_t target_start = loop_start;
    memcpy(&chk.code[jump_back], &target_start, sizeof(uint64_t));

    patchJump(jump_exit);

    for (size_t i = 0; i < loop_stack[loop_depth].count; i++) {
      patchJump(loop_stack[loop_depth].jumps[i]);
    }
    loop_depth--;

    break;
  }
  case NODE_FOR_LOOP: {
    loop_depth++;
    loop_stack[loop_depth].count = 0;

    if (node->data.for_loop.init) {
      compileNode(node->data.for_loop.init);
    }
    size_t loop_start = chk.count;
    size_t jump_exit = 0;
    if (node->data.for_loop.condition) {
      compileExpr(node->data.for_loop.condition);
      jump_exit = emitJump(OP_JUMP_F);
    }
    for (size_t i = 0; i < node->data.for_loop.s_body; i++) {
      compileNode(node->data.for_loop.body[i]);
    }
    if (node->data.for_loop.inc) {
      compileNode(node->data.for_loop.inc);
    }
    size_t jump_back = emitJump(OP_JUMP);
    uint64_t target_start = loop_start;
    memcpy(&chk.code[jump_back], &target_start, sizeof(uint64_t));

    if (node->data.for_loop.condition) {
      patchJump(jump_exit);
    }

    for (size_t i = 0; i < loop_stack[loop_depth].count; i++) {
      patchJump(loop_stack[loop_depth].jumps[i]);
    }
    loop_depth--;

    break;
  }
  case FUNCTION_DEF: {
    size_t skip_jump = emitJump(OP_JUMP);
    size_t fn_start = chk.count;

    compileNode(node->data.fn_def.program);

    writeChunk(&chk, OP_PUSH_UINT);
    writeChunkQ(&chk, (uint64_t)fn_start);
    writeChunk(&chk, OP_RET);

    patchJump(skip_jump);

    // 1. Empujamos el contexto / ámbito actual (PSELF)
    writeChunk(&chk, OP_PSELF);

    // 2. Cargamos el nombre de la función como constante
    ShellValue name_val = newStrVal(node->data.fn_def.name);
    int c_idx = addConstant(&chk, name_val);
    if (c_idx > 255) {
      writeChunk(&chk, OP_LOAD_W_CONSTANT);
      writeChunk(&chk, c_idx & 0xFF);
      writeChunk(&chk, (c_idx >> 8) & 0xFF);
    } else {
      writeChunk(&chk, OP_LOAD_CONSTANT);
      writeChunk(&chk, (uint8_t)c_idx);
    }

    // 3. Purgamos el truco del UINT y metemos la instrucción que crea el objeto
    // función encapsulado
    writeChunk(&chk, OP_PUSH_UINT);
    writeChunkQ(&chk, (uint64_t)fn_start);
    writeChunk(&chk, OP_MAKE_FUNC);

    // 4. Definimos la variable en el contexto
    writeChunk(&chk, OP_DEF);
    break;
  }
  case CLASS_DEF: {
    size_t skip_jump = emitJump(OP_JUMP);
    size_t class_start = chk.count;

    compileNode(node->data.class_def.program);

    writeChunk(&chk, OP_PSELF);
    writeChunk(&chk, OP_RET);

    patchJump(skip_jump);

    writeChunk(&chk, OP_PSELF);
    ShellValue name_val = newStrVal(node->data.class_def.name);
    int c_idx = addConstant(&chk, name_val);
    if (c_idx > 255) {
      writeChunk(&chk, OP_LOAD_W_CONSTANT);
      writeChunk(&chk, c_idx & 0xFF);
      writeChunk(&chk, (c_idx >> 8) & 0xFF);
    } else {
      writeChunk(&chk, OP_LOAD_CONSTANT);
      writeChunk(&chk, (uint8_t)c_idx);
    }

    writeChunk(&chk, OP_PUSH_UINT);
    writeChunkQ(&chk, (uint64_t)class_start);
    writeChunk(&chk, OP_MAKE_FUNC);
    writeChunk(&chk, OP_DEF);
    break;
  }
  case NODE_RETURN: {
    if (node->data.ret_stmt.expr != NULL) {
      compileExpr(node->data.ret_stmt.expr);
    } else {
      writeChunk(&chk, OP_PUSH_UINT);
      writeChunkQ(&chk, 0);
    }
    writeChunk(&chk, OP_RET);
    break;
  }
  case NODE_BREAK: {
    if (loop_depth >= 0) {
      size_t jmp = emitJump(OP_JUMP);
      if (loop_stack[loop_depth].count < 64) {
        loop_stack[loop_depth].jumps[loop_stack[loop_depth].count++] = jmp;
      }
    }
    break;
  }
  case NODE_COMMAND_CALL: {
    break;
  }
  case FUNCTION_CALL: {
    compileExpr(node);
    break;
  }
  default: {
    compileExpr(node);
    break;
  }
  }
}

static void compileExpr(ASTNode *expr) {
  if (expr == NULL)
    return;

  switch (expr->type) {
  case NODE_NUM_LIT: {
    double num = expr->data.number_lit;
    uint64_t bits;
    memcpy(&bits, &num, sizeof(double));
    writeChunk(&chk, OP_PUSH);
    writeChunkQ(&chk, bits);
    break;
  }
  case NODE_STRING_LIT: {
    ShellValue str_val = ref_add(newStrVal(expr->data.string_lit));
    int c_idx = addConstant(&chk, str_val);
    if (c_idx > 255) {
      writeChunk(&chk, OP_LOAD_W_CONSTANT);
      writeChunk(&chk, c_idx & 0xFF);
      writeChunk(&chk, (c_idx >> 8) & 0xFF);
    } else {
      writeChunk(&chk, OP_LOAD_CONSTANT);
      writeChunk(&chk, (uint8_t)c_idx);
    }
    break;
  }
  case NODE_VAR_REF: {
    emitMemberPath(expr->data.var_ref.name, expr->data.var_ref.member,
                   countMembers(expr->data.var_ref.member));
    break;
  }
  case NODE_ARG_REF: {
    writeChunk(&chk, OP_GET_ARG);
    writeChunkQ(&chk, (uint64_t)expr->data.arg_ref.num);
    break;
  }
  case FUNCTION_CALL: {
    if (expr->data.fn_call.member == NULL) {
      writeChunk(&chk, OP_PSELF);
      // Asegurarnos de que pasamos el nombre real de la función y nunca un
      // punto o vacío
      if (expr->data.fn_call.name != NULL) {
        emitStringConstant(expr->data.fn_call.name);
      } else {
        emitStringConstant("");
      }
      writeChunk(&chk, OP_GET);
    } else {
      emitMemberPath(expr->data.fn_call.name, expr->data.fn_call.member,
                     countMembers(expr->data.fn_call.member));
    }

    size_t argc = expr->data.fn_call.argc;
    for (size_t i = 0; i < argc; i++) {
      compileExpr(expr->data.fn_call.args[i]);
    }

    writeChunk(&chk, OP_PUSH_UINT);
    writeChunkQ(&chk, (uint64_t)argc);
    writeChunk(&chk, OP_CALL);
    break;
  }
  case NODE_BINARY_EXPR: {
    compileExpr(expr->data.binary_expr.left);
    compileExpr(expr->data.binary_expr.right);

    OP op = OP_ADD;
    switch (expr->data.binary_expr.op) {
    case TOKEN_PLUS:
      op = OP_ADD;
      break;
    case TOKEN_MINUS:
      op = OP_SUB;
      break;
    case TOKEN_MUL:
      op = OP_MUL;
      break;
    case TOKEN_DIV:
      op = OP_DIV;
      break;
    case TOKEN_LESS:
      op = OP_LT;
      break;
    case TOKEN_GREATER:
      op = OP_GT;
      break;
    case TOKEN_LEQ:
      op = OP_LEQ;
      break;
    case TOKEN_GEQ:
      op = OP_GEQ;
      break;
    case TOKEN_EQ:
      op = OP_EQ;
      break;
    case TOKEN_BEQ:
      op = OP_NEQ;
      break;
    case TOKEN_AND:
      op = OP_AND;
      break;
    case TOKEN_OR:
      op = OP_OR;
      break;
    default:
      break;
    }
    writeChunk(&chk, op);
    break;
  }
  case NODE_UNARY: {
    compileExpr(expr->data.unary_expr.expr);
    if (expr->data.unary_expr.op == TOKEN_BANG) {
      writeChunk(&chk, OP_NOT);
    }
    break;
  }
  default:
    break;
  }
}
