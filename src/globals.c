#include "../include/globals.h"
#include "../include/eval.h"
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include "../include/lexer.h"
#include "../include/parser.h"
#include <stdlib.h>

typedef enum {
    PTR_RAW,       // kfree plano (para strings sueltos o buffers)
    PTR_AST,       // parser_free_ast para el árbol
    PTR_TOKENS     // tu función del lexer para liberar tokens y sus textos internos
} PtrType;

typedef struct {
    void *ptr;
    PtrType type;
} TrackedItem;

typedef struct {
    TrackedItem *items;
    size_t count;
    size_t capacity;
} MemTracker;

MemTracker *active_tracker = NULL;

void init_tracker() {
    active_tracker = kmalloc(sizeof(MemTracker));
    active_tracker->count = 0;
    active_tracker->capacity = 64;
    active_tracker->items = kmalloc(sizeof(TrackedItem) * active_tracker->capacity);
}

void track_pointer(void *ptr, PtrType type) {
    if (active_tracker == NULL || ptr == NULL) return;
    
    if (active_tracker->count >= active_tracker->capacity) {
        active_tracker->capacity *= 2;
        TrackedItem *new_items = kmalloc(sizeof(TrackedItem) * active_tracker->capacity);
        memcpy(new_items, active_tracker->items, sizeof(TrackedItem) * active_tracker->count);
        kfree(active_tracker->items);
        active_tracker->items = new_items;
    }
    
    active_tracker->items[active_tracker->count++] = (TrackedItem){.ptr = ptr, .type = type};
}

void free_tracked_memory() {
    if (active_tracker == NULL) return;
    
    for (size_t i = 0; i < active_tracker->count; i++) {
        TrackedItem item = active_tracker->items[i];
        if (item.ptr != NULL) {
            if (item.type == PTR_AST) {
                parser_free_ast((ASTNode*)item.ptr);
            } else if (item.type == PTR_TOKENS) {
                lexer_free((Token*)item.ptr); // O free_tokens() según tu cabecera
            } else {
                kfree(item.ptr);
            }
        }
    }
    
    kfree(active_tracker->items);
    kfree(active_tracker);
    active_tracker = NULL;
}

int how_many=0;

CFunc natives[MAX_NATIVE_FUNCS];

ShellValue native_import(int argc, ShellValue *args) {
	if (argc < 1) return newNumVal(0);
	char *str = args[0].as.str;
	FILE *f = fopen(str, "r");
	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0, SEEK_SET);
	char *buffer = kmalloc(size+1);
	fread(buffer, 1, size, f);
	buffer[size] = '\0';

	EvalCtx *current = newShellCtx();
	lexer_init(buffer);
	Token *t_buff = lexe(NULL);
	parser_init(t_buff);
	ASTNode *tree=parse();

	eval(tree, current);
	track_pointer(tree, PTR_AST);
	track_pointer(t_buff, PTR_TOKENS);
	track_pointer(t_buff, PTR_RAW);

	ShellValue val = newInstanceVal(current);
	free(buffer);
	return val;
}

void cleanAll() {
	free_tracked_memory();
}

ShellValue native_write(int argc, ShellValue *args) {
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        return newNumVal(0);
    }
    
    FILE *f = fopen(args[0].as.str, "w");
    if (!f) return newNumVal(0);
    
    fputs(args[1].as.str, f);
    fclose(f);
    return newNumVal(1); // Éxito, YUPIIII
}

// read("algo.txt")
ShellValue native_read(int argc, ShellValue *args) {
    if (argc < 1 || args[0].type != VAL_STRING) {
        return newStrVal("");
    }
    
    FILE *f = fopen(args[0].as.str, "r");
    if (!f) return newStrVal("");

    // a ver, en plan solo asignar lo q necesito
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = kmalloc(len + 1);
    if (!buf) {
        fclose(f);
        return newStrVal("");
    }
    
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);

    ShellValue val = newStrVal(buf);
    kfree(buf);
    return val;
}

ShellValue native_input(int argc, ShellValue *args) {
    char buffer[256];
    if (argc > 0 && args[0].type == VAL_STRING) {
        kprintf("%s", args[0].as.str); 
    }
    
    
    if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }
        return newStrVal(buffer);
    }
    return newStrVal("NULL");
}

ShellValue native_array_push(int argc, ShellValue *args) {
    
    if (argc < 2) return newNumVal(0);
    
    InstanceObj *inst = (InstanceObj*)((size_t)args[0].as.comodin - sizeof(GCObj));


    EvalCtx *inst_ctx = inst->ctx;

    ShellValue val = (getShellVarEntrySilent(inst_ctx, "data"))->val;
    Array *arr = val.as.arr;
    if (arr->size >= arr->capacity) growArray(arr);
    arr->items[arr->size++]=ref_add(args[1]);
    return newNumVal(0);
}

ShellValue native_array_pop(int argc, ShellValue *args) {
    
    if (argc < 1) return newNumVal(0);
    InstanceObj *inst = (InstanceObj*)((size_t)args[0].as.comodin - sizeof(GCObj));


    EvalCtx *inst_ctx = inst->ctx;

    ShellValue val = (getShellVarEntrySilent(inst_ctx, "data"))->val; 
    Array *arr = val.as.arr;
    arr->size--;
    ref_del(arr->items[arr->size]);
    return arr->items[arr->size];
}

ShellValue native_array_len(int argc, ShellValue *args) { 
    if (argc < 1) return newNumVal(0);
    InstanceObj *inst = (InstanceObj*)((size_t)args[0].as.comodin - sizeof(GCObj));


    EvalCtx *inst_ctx = inst->ctx;

    ShellValue val = (getShellVarEntrySilent(inst_ctx, "data"))->val; 
    Array *arr = val.as.arr;
    return newNumVal(arr->size);
}


ShellValue native_array_constructor(int argc, ShellValue *args) {
    
    EvalCtx *instance_ctx = newShellCtx();
    
    ShellValue arr_val = newArrayVal(); 
    
    defShellVarEntry(instance_ctx, "data", arr_val);

    
    register_cfunc(instance_ctx, "push", native_array_push);
    register_cfunc(instance_ctx, "pop", native_array_pop);
    register_cfunc(instance_ctx, "len", native_array_len);


    return newInstanceVal(instance_ctx);
}

ShellValue len(int argc, ShellValue *args) {
	if (argc < 1) return newNumVal(0);
	if (args[0].type=VAL_STRING) {
		return newNumVal(strlen(args[0].as.str));
	}
	return newNumVal(0);
}

ShellValue zindex(int argc, ShellValue *args) {
		if (argc < 2) return newNumVal(0);
		if (args[0].type==VAL_STRING) {
				char str[2];
				str[1] = '\0';
				str[0] = args[0].as.str[(int)args[1].as.num];
				return newStrVal(str);
		} else if (args[0].type==VAL_ARRAY) {
			return args[0].as.arr->items[(int)args[1].as.num];
		}
		return newNumVal(0);
}

void register_cfunc(EvalCtx *ctx, char *name, NativeFunc func) {
	ShellValue val;
	val.type = VAL_CFUNC;
	CFunc *var=&natives[how_many++];
	var->func = func;
	val.as.c_func = var;
	defShellVarEntry(ctx, name, val);
}

void register_globals(EvalCtx *ctx) {
	register_cfunc(ctx, "len", len);
	register_cfunc(ctx, "index", zindex);
	register_cfunc(ctx, "Array", native_array_constructor);
	register_cfunc(ctx, "input", native_input);
	register_cfunc(ctx, "read", native_read);
	register_cfunc(ctx, "write", native_write);
	register_cfunc(ctx, "import", native_import);
}
