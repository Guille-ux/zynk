#ifndef SHELL_EVAL_H
#define SHELL_EVAL_H

#include "mbtype.h"
#include "common.h"
#include "parser.h"
#include "lexer.h"
#include <string.h>

struct EvalCtx;

#define ZYNK_MAGIC_CLASS    0x434C4153
#define ZYNK_MAGIC_ARRAY    0x434C4152



typedef enum {
	VAL_NONE = 0,
	VAL_NUMBER = 1,
	VAL_STRING = 2,
	VAL_FUNC = 3,
	VAL_CFUNC = 4,
	VAL_DICT = 5, // las clases seran diccionarios JAJAJJAJAJAJJA
	VAL_ARRAY = 6,
	VAL_OBJ = 7, // objeto no estandar
} ValType;

struct ZFunc;
struct ShellValue;
struct Object;
struct CFunc;

typedef struct {
	uint32_t refc;
	uint32_t magic;
} Obj;

typedef struct GCObj {
	struct GCObj *last; // el anterior
	struct GCObj *next;
	Obj obj;
} GCObj;

struct CFunc;
struct ArrayObj;
struct Array;



typedef struct Object {
	int capacity;
	uint8_t info[];
} Object;

typedef struct {
	GCObj data;
	int capacity;
	uint8_t info[];
} RealObj;

typedef struct ShellValue {
	ValType type;
	union {
		double num;
		char *str;
		void *comodin;
		struct CFunc *c_func;
		struct ZFunc *func;
		struct Object *obj;
		struct Array *arr;
	} as;
	uint32_t refc; // no se
} ShellValue;

typedef ShellValue (*NativeFunc)(int argc, ShellValue *args);

typedef struct {
    GCObj data;
    int size;
    int capacity;
    ShellValue *items; // Un array plano de valores de Zynk
} ArrayObj;

typedef struct Array {
	int size;
	int capacity;
	ShellValue *items;
} Array;

typedef struct CFunc {
	NativeFunc func;
} CFunc;

typedef struct {
	Obj data;
	char str[];
} String;

typedef struct {
	Obj data;
	struct EvalCtx *ctx;
} Namespace;

typedef struct ZFunc {
	struct EvalCtx *ctx;
	ASTNode *program;
} ZFunc;

typedef struct {
	GCObj data;
	struct EvalCtx *ctx;
	ASTNode *program;
} ZFuncObj;

typedef struct {
	char *name;
	ShellValue val;
	bool is_exported;
	bool is_set;
} Var;

struct EvalCtx;

typedef struct {
	GCObj data;
	struct EvalCtx *ctx;
	ASTNode *program;
} ClassObj;

typedef struct {
	GCObj data;
	struct EvalCtx *ctx;
} InstanceObj;



ShellValue newClassVal(struct EvalCtx *ctx, ASTNode *program);
ShellValue newInstanceVal(struct EvalCtx *ctx);


typedef struct EvalCtx {
	struct EvalCtx *parent;
	struct EvalCtx *global;
	size_t arg_used;
	Var *var_table;
	Var *argv;
	size_t argc;
	size_t var_cap;
	
	int (*command_handler)(ASTNode *command, struct EvalCtx *ctx);

	int last_exit_code;

	bool should_break;
	bool should_ret;
	ShellValue ret;
	bool should_exit;
	
	int exit_code;
} EvalCtx;

void growArray(Array *array);
ShellValue newObjVal(int size);
ShellValue newArrayVal();
ShellValue newNumVal(double num);
ShellValue newStrVal(const char *str);
void freeVal(ShellValue val);
void addShellArg(EvalCtx *ctx, ShellValue val);
ShellValue getShellArg(EvalCtx *ctx, size_t id);

void addObj(GCObj *obj);
void delObj(GCObj *obj);
void zynk_gc();

/*
 * Status Codes
 * 
 * - 0 Ok
 * - 1 Err
 * - 2 Break Statement
 * - 3 Return Statement
 * - 4 Exit Statement (not implemented yet)
 *
 */

EvalCtx *get_global();
void set_global(EvalCtx *ctx);
EvalCtx *newShellCtx();
void midKillShellCtx(EvalCtx *ctx);
void killShellCtx(EvalCtx *ctx);
Var *getShellVarEntry(EvalCtx *ctx, char *name);
Var *getShellVarEntrySilent(EvalCtx *ctx, char *name);
void setShellVarEntry(EvalCtx *ctx, char *name, ShellValue val);
void defShellVarEntry(EvalCtx *ctx, char *name, ShellValue val);
void delShellVarEntry(EvalCtx *ctx, char *name);
uint32_t zhash(char *name); // aham, a futuro

ShellValue ref_add(ShellValue val);
void ref_del(ShellValue val);
ShellValue evalExpr(ASTNode *expr, EvalCtx *ctx);
int eval(ASTNode *ast, EvalCtx *ctx);

#endif
