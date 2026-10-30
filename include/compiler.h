#ifndef ZYNK_COMPILER_H
#define ZYNK_COMPILER_H

#include "vm.h"
#include "chunk.h"
void disassembleChunk(Chunk *chunk, const char *name);

int compile(ASTNode *node);

#endif
