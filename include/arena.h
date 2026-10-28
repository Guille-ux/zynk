#ifndef ZYNK_ARENA_H
#define ZYNK_ARENA_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHUNK_SIZE (128  *1024)


typedef struct ArenaChunk {
    struct ArenaChunk *next;
    size_t alloc_ptr;
    
    char data[]; 
} ArenaChunk;

typedef struct {
    ArenaChunk *current;
} Arena;


Arena arena_create();


void *arena_alloc(Arena *arena, size_t size);


void arena_free(Arena *arena);

extern Arena g_arena;

void initArena();
void *amalloc(size_t size);
void afree();


#endif
