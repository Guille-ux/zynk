#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/arena.h"

Arena g_arena;

Arena arena_create() {
    ArenaChunk *first = malloc(sizeof(ArenaChunk) + CHUNK_SIZE);
    if (!first) {
        fprintf(stderr, "Err -> without mem for initial arena\n");
        exit(1);
    }
    first->next = NULL;
    first->alloc_ptr = 0;
    return (Arena){ .current = first };
}


void *arena_alloc(Arena* arena, size_t size) {
    
    size = (size + 7) & ~7;

    
    if (arena->current->alloc_ptr + size > CHUNK_SIZE) {
        
        ArenaChunk *new_chunk = malloc(sizeof(ArenaChunk) + CHUNK_SIZE);
        if (!new_chunk) {
            fprintf(stderr, "Couldn't grow the Arena Allocator\n");
            exit(1);
        }
        new_chunk->alloc_ptr = 0;
        
        
        new_chunk->next = arena->current;
        arena->current = new_chunk;
    }

    
    void *ptr = &arena->current->data[arena->current->alloc_ptr];
    
    
    arena->current->alloc_ptr += size;
    
    return ptr;
}


void arena_free(Arena *arena) {
    ArenaChunk *curr = arena->current;
    while (curr != NULL) {
        ArenaChunk *next = curr->next;
        free(curr); 
        curr = next;
    }
    arena->current = NULL;
}

void initArena() {
	g_arena = arena_create();
}

void *amalloc(size_t size) {
	return arena_alloc(&g_arena, size);
}

void afree() {
	arena_free(&g_arena);
}
