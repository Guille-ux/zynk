#ifndef ZYNK_CHUNK_H
#define ZYNK_CHUNK_H

#include "eval.h"
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    size_t count;
    size_t capacity;
    uint8_t *code;       
    
    size_t constants_count;
    size_t constants_capacity;
    ShellValue *constants; 
} Chunk;

void initChunk(Chunk *chunk);
void writeChunk(Chunk *chunk, uint8_t byte);
void writeChunkQ(Chunk* chunk, uint64_t val);
int addConstant(Chunk *chunk, ShellValue value);

#endif
