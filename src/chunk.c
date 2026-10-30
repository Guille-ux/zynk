#include "../include/chunk.h"

void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->constants_count = 0;
    chunk->constants_capacity = 0;
    chunk->constants = NULL;
}


void writeChunk(Chunk* chunk, uint8_t byte) {
    if (chunk->capacity < chunk->count + 1) {
        chunk->capacity = chunk->capacity < 8 ? 8 : chunk->capacity * 2;
        chunk->code = realloc(chunk->code, chunk->capacity * sizeof(uint8_t));
    }
    chunk->code[chunk->count++] = byte;
}

void writeChunkQ(Chunk* chunk, uint64_t val) {
    
    if (chunk->capacity < chunk->count + 8) {
        chunk->capacity = chunk->capacity < 8 ? 8 : chunk->capacity * 2;
        chunk->code = realloc(chunk->code, chunk->capacity * sizeof(uint8_t));
    }
    
    
    uint64_t *dest = (uint64_t*)&chunk->code[chunk->count];
    *dest = val;
    
    
    chunk->count += 8;
}


int addConstant(Chunk* chunk, ShellValue value) {
    if (chunk->constants_capacity < chunk->constants_count + 1) {
        chunk->constants_capacity = chunk->constants_capacity < 8 ? 8 : chunk->constants_capacity * 2;
        chunk->constants = realloc(chunk->constants, chunk->constants_capacity * sizeof(ShellValue));
    }
    chunk->constants[chunk->constants_count] = value;
    return chunk->constants_count++;
}

