#include "../include/globals.h"
#include "../include/eval.h"
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include "../include/lexer.h"
#include "../include/parser.h"
#include <stdlib.h>

ShellValue native_getenv(int argc, ShellValue *args) {
	if (argc < 1) return newNumVal(-1);
	char *ptr = getenv(args[0].as.str);
	ShellValue val = newStrVal(ptr);
	return val;
}

ShellValue native_max(int argc, ShellValue *args) {
	if (argc < 2) return newNumVal(-1);
	return (args[0].as.num > args[1].as.num) ? args[0] : args[1];
}
ShellValue native_min(int argc, ShellValue *args) {
	if (argc < 2) return newNumVal(-1);
	return (args[0].as.num < args[1].as.num) ? args[0] : args[1];
}

ShellValue native_newb(int argc, ShellValue *args) {
	if (argc < 1) return newNumVal(-1);
	return newObjVal((size_t)args[0].as.num);
}

ShellValue native_setb(int argc, ShellValue *args) {
	if (argc < 3) return newNumVal(-1);
	Object *obj = args[0].as.obj;
	size_t pos = (size_t)args[1].as.num;
	uint8_t new_val = (uint8_t)args[2].as.num;
	obj->info[pos] = new_val; // yay
	return newNumVal(0);
}

ShellValue native_getb(int argc, ShellValue *args) {
	if (argc < 2) return newNumVal(-1);
	Object *obj = args[0].as.obj;
	size_t pos = (size_t)args[1].as.num;
	// yippppppieeeee, idk
	return newNumVal((double)obj->info[pos]);
}
/*
 *
 * This functions will come soon, yk, i think they would be useful
 * idk
 *
 *
ShellValue native_text2b(int argc, ShellValue *args) {
	if (argc < 1) return newNumVal(-1);
	ShellValue val = newObjVal(strlen(args[0].as.str));

}

ShellValue native_b2text(int argc, ShellValue *args) {
	if (argc < 1) return newNumVal(-1);
	if (args[0].type!=VAL_OBJ) return newNumVal(-1);
	Object *obj = 
	return newStrVal();
}
*/
ShellValue native_chr(int argc, ShellValue *args) {
	if (argc < 1) return newNumVal(-1);
	char buffi[2];
	buffi[0] = (char)args[0].as.num;
	buffi[1] = '\0';
	return newStrVal(buffi);
}

ShellValue native_ord(int argc, ShellValue *args) {
	if (argc < 1) return newNumVal(-1);
	char ch = args[0].as.str[0]; // pillamos el primer caractér
	return newNumVal((double)ch);
}

ShellValue native_float(int argc, ShellValue *args) {
	if (argc < 1) return newNumVal(-1);
	return newNumVal(atof(args[0].as.str));
}

ShellValue native_str(int argc, ShellValue *args) {
	if (argc < 1) return newNumVal(-1);
	char buffer[512];
	snprintf(buffer, sizeof(buffer), "%lf", args[0].as.num);
	buffer[511] = '\0';
	return newStrVal(buffer);
}

ShellValue native_args(int argc, ShellValue *args) {
	if (argc < 1) return newNumVal(-1);
	return getShellArg(get_global(), (size_t)args[0].as.num);
}
	

// ahora algo del tiempo, para los benchmarks
#include <sys/time.h>
ShellValue native_clock(int argc, ShellValue *args) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    double seconds = (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
    return newNumVal(seconds);
}

typedef void (*RegisterLibFunc)(EvalCtx *ctx);

ShellValue native_system(int argc, ShellValue *args) {
	if (argc < 1) return newNumVal(0);
	char *str = args[0].as.str;
	system(str);
	return newNumVal(1);
}

#ifdef _WIN32
#include <windows.h>
typedef void (*RegisterLibFunc)(EvalCtx *ctx);

int load_plugins(const char *file, EvalCtx *ctx) {
 
    HMODULE handle = LoadLibraryA(file);
    if (!handle) {
        printf("Err -> Error while Loading .dll\n");
        return 0;
    }

    RegisterLibFunc register_lib = (RegisterLibFunc)GetProcAddress(handle, "register_lib");
    if (!register_lib) {
        printf("Err -> Error, 'register_lib' not found\n");
        FreeLibrary(handle);
        return 0;
    }

    register_lib(ctx);
    return 0;
}
#else
#include <dlfcn.h>

int load_plugins(const char *file, EvalCtx *ctx) {
    void *handle = dlopen(file, RTLD_NOW);
    if (!handle) {
        printf("Err -> Error while Loading .so\n");
        return 0;
    }
    dlerror();

    RegisterLibFunc register_lib = (RegisterLibFunc)dlsym(handle, "register_lib");
    char *error = dlerror();
    if (error != NULL) {
        printf("Err -> Error, 'register_lib' not found\n");
        dlclose(handle);
        return 0;
    }

    register_lib(ctx);
    return 0;
}
#endif

ShellValue native_include(int argc, ShellValue *args) {
	if (argc < 1) return newNumVal(0);
	char *str = args[0].as.str;
	load_plugins(str, get_global());
	return newNumVal(1);
}

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
    return newNumVal(0);
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
	if (args[0].type==VAL_STRING) {
		return newNumVal(strlen(args[0].as.str));
	} else if (args[0].type==VAL_OBJ) {
		return newNumVal((double)args[0].as.obj->capacity);
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

void register_class(EvalCtx *ctx, char *name, EvalCtx *class) {	
	defShellVarEntry(ctx, name, newInstanceVal(class));
}

void register_globals(EvalCtx *ctx) {
	register_cfunc(ctx, "len", len);
	register_cfunc(ctx, "index", zindex);
	register_cfunc(ctx, "Array", native_array_constructor);
	register_cfunc(ctx, "input", native_input);
	register_cfunc(ctx, "read", native_read);
	register_cfunc(ctx, "write", native_write);
	register_cfunc(ctx, "import", native_import);
	register_cfunc(ctx, "include", native_include);
	register_cfunc(ctx, "system", native_system);
	register_cfunc(ctx, "clock", native_clock);
	register_cfunc(ctx, "arg", native_args);
	register_cfunc(ctx, "float", native_float);
	register_cfunc(ctx, "str", native_str);
	register_cfunc(ctx, "chr", native_chr);
	register_cfunc(ctx, "ord", native_ord);
	register_cfunc(ctx, "newb", native_newb);
	register_cfunc(ctx, "setb", native_setb);
	register_cfunc(ctx, "getb", native_getb);
	register_cfunc(ctx, "max", native_max);
	register_cfunc(ctx, "min", native_min);
	register_cfunc(ctx, "getenv", native_getenv);
}
