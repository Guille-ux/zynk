#include "../include/eval.h"
#include "../include/modules.h"

// Value
static ShellValue evalNum(ASTNode *num);
static ShellValue evalStr(ASTNode *str);
static ShellValue evalVarRef(ASTNode *ref, EvalCtx *ctx);
static ShellValue evalArgRef(ASTNode *ref, EvalCtx *ctx);
static ShellValue evalBinExpr(ASTNode *expr, EvalCtx *ctx);
static ShellValue evalUnaryExpr(ASTNode *expr, EvalCtx *ctx);
static EvalCtx *evalMembers(ASTNode *stmt, char *name, EvalCtx *ctx, char **oname);
static ShellValue evalFuncCall(ASTNode *stmt, EvalCtx *ctx);

// Status Code
static int evalClassDef(ASTNode *stmt, EvalCtx *ctx);
static int evalFuncDef(ASTNode *stmt, EvalCtx *ctx);
static int evalStatement(ASTNode *stmt, EvalCtx *ctx);
static int evalProgram(ASTNode *program, EvalCtx *ctx);
static int evalCommandCall(ASTNode *call, EvalCtx *ctx);
static int evalAssignment(ASTNode *stmt, EvalCtx *ctx);
static int evalIf(ASTNode *stmt, EvalCtx *ctx);
static int evalWhile(ASTNode *stmt, EvalCtx *ctx);
static int evalFor(ASTNode *stmt, EvalCtx *ctx);
static int evalEcho(ASTNode *stmt, EvalCtx *ctx);
static int evalVarDef(ASTNode *stmt, EvalCtx *ctx);
static int evalVarDel(ASTNode *stmt, EvalCtx *ctx);

#define EVAL_OK 0
#define EVAL_ERR 1
#define EVAL_BREAK 2
#define EVAL_RET 3
#define EVAL_EXIT 4

EvalCtx *global;
GCObj *obj_pool;
int cycles=0;
#define CYCLES_PER_SWEEP 100 // es decir 100 expresiones entre cada
			     // pasada



void set_global(EvalCtx *ctx) {
	ctx->global = NULL;
	global = ctx;
}

EvalCtx *get_global() {
	return global;
}

ShellValue newNumVal(double num) {
	ShellValue val;
	val.type = VAL_NUMBER;
	val.as.num = num;
	return val;
}

// FNV-1a 32-bit hash function (Public Domain - Fowler/Noll/Vo)
uint32_t zhash(char *name) {
    uint32_t hash = 2166136261u;
    while (*name) {
        hash ^= (uint8_t)(*name++);
        hash *= 16777619u;
    }
    return hash;
}

void addObj(GCObj *obj) {
	GCObj *current = obj_pool;
	obj_pool = obj;
	obj_pool->next = current;
}

void delObj(GCObj *obj) {
	if (obj->last==NULL) {
		obj_pool=obj->next;
	} else {
		obj->last->next = obj->next;
	}
	if (obj->obj.magic==ZYNK_MAGIC_CLASS) {
		void *ptr = (void*)((size_t)obj+sizeof(GCObj));
		killShellCtx((EvalCtx*)ptr);
	}
	if (obj->obj.magic==ZYNK_MAGIC_ARRAY) {
		ArrayObj *obj = (ArrayObj*)obj;
		void *ptr = (void*)obj->items;
		if (obj->capacity==0) {

		} else {
			kfree(ptr);
		}
	}
	kfree(obj); // lo limpiamos, perfecto! NICE
}

void growArray(Array *array) {
	ShellValue *new_items = kmalloc(array->capacity*2*sizeof(ShellValue));
	memcpy(new_items, array->items, array->size);
	array->capacity *= 2;
	kfree(array->items);
	array->items = new_items;
}

void zynk_gc() {
	GCObj *current = obj_pool;
	while (current!=NULL) {
		if (current->obj.refc==0) {
			GCObj *next = current->next;
			delObj(current);
			current = next;
		} else {
			current = current->next;
		}
	}
}

bool isHeap(void *ptr) {
	GCObj *current = obj_pool;
	while (current!=NULL) {
		if (((size_t)(&current->obj)+sizeof(Obj))==(size_t)ptr) {
			return true;
		}
		current = current->next;
	}
	return false;
}

ShellValue newObjVal(int size) {
	ShellValue val;
	val.type = VAL_OBJ;
	RealObj *obj = (RealObj*)kmalloc(sizeof(RealObj)+size);
	memset(obj, 0, sizeof(RealObj)+size);
	val.as.obj = (Object*)((size_t)obj+sizeof(GCObj));
	obj->capacity = size;
	addObj((GCObj*)obj);
	return val;
}

ShellValue newFuncVal(EvalCtx *ctx, ASTNode *program) {
	ShellValue val;
	val.type = VAL_FUNC;
	ZFuncObj *func = (ZFuncObj*)kmalloc(sizeof(ZFuncObj));
	memset(func, 0, sizeof(ZFuncObj));
	val.as.func = (ZFunc*)((size_t)func + sizeof(GCObj));
	if (func==NULL) {
		kprintf("Err -> Out of Memory\n");
		val.type=VAL_NONE;
	}
	func->ctx = ctx;
	func->program = program;
	func->data.obj.refc = 0;
	addObj((GCObj*)func);
	return val;
}


ShellValue newClassVal(EvalCtx *ctx, ASTNode *program) {
	ShellValue val;
	val.type = VAL_DICT;
	ClassObj *klass = (ClassObj*)kmalloc(sizeof(ClassObj));
	memset(klass, 0, sizeof(ClassObj));
	val.as.comodin = (void*)((size_t)klass + sizeof(GCObj));
	klass->ctx = ctx;
	klass->program = program;
	klass->data.obj.refc = 0;
	klass->data.obj.magic = ZYNK_MAGIC_CLASS;
	addObj((GCObj*)klass);
	return val;
}


ShellValue newInstanceVal(EvalCtx *ctx) {
	ShellValue val;
	val.type = VAL_DICT;
	InstanceObj *instance = (InstanceObj*)kmalloc(sizeof(InstanceObj));
	memset(instance, 0, sizeof(InstanceObj));
	val.as.comodin = (void*)((size_t)instance + sizeof(GCObj));
	instance->ctx = ctx;
	instance->data.obj.refc = 0;
	instance->data.obj.magic = ZYNK_MAGIC_CLASS;
	addObj((GCObj*)instance);
	return val;
}


static int evalClassDef(ASTNode *stmt, EvalCtx *ctx) {
	ShellValue class_val = newClassVal(ctx, stmt->data.class_def.program);
	defShellVarEntry(ctx, stmt->data.class_def.name, class_val);
	return 0;
}

ShellValue newArrayVal() {
	ShellValue val;
	val.type = VAL_ARRAY;
	int size = 0;
	int capacity = 10; // no se un número inicial
	GCObj *arr = (GCObj*)kmalloc(sizeof(ArrayObj));
	memset(arr, 0, sizeof(ArrayObj));
	ArrayObj *a = (ArrayObj*)arr;
	a->size = size;
	a->capacity = capacity;
	a->items = (ShellValue*)kmalloc(sizeof(ShellValue)*capacity);
	val.as.arr = (Array*)((size_t)a+sizeof(GCObj));
	arr->obj.magic = ZYNK_MAGIC_ARRAY;
	addObj(arr);
	return val;
}

ShellValue newStrVal(const char *str) {
	ShellValue val;
	val.type = VAL_STRING;
	uint32_t len = strlen(str);
	GCObj *string = (GCObj*)kmalloc(sizeof(GCObj)+len+1);
	memset(string, 0, sizeof(GCObj)+len+1);
	val.as.str = (char*)((size_t)string + sizeof(GCObj));
	if (string==NULL) {
		kprintf("Err -> Out of Memory\n");
		val.type=VAL_NONE;
	}
	strcpy(val.as.str, str);
	val.as.str[len]='\0';
	string->obj.refc = 0;
	addObj(string);
	return val;
}

void freeVal(ShellValue val) {
	if (val.type == VAL_STRING ||
	    val.type == VAL_DICT ||
	    val.type == VAL_FUNC) {
		GCObj *string = (GCObj*)((size_t)val.as.comodin - sizeof(GCObj));
		kfree(string);
		val.as.str = NULL;
	}
}

void midKillShellCtx(EvalCtx *ctx) {
	if (ctx==NULL) return;

	for (size_t i=0;i<ctx->var_cap;i++) {
		if (ctx->var_table[i].is_set && ctx->var_table[i].name!=NULL && ctx->var_table[i].val.as.comodin!=NULL) {
			ref_del(ctx->var_table[i].val);
		}
	}
	for (size_t i=0;i<ctx->arg_used;i++) {
		ref_del(ctx->argv[i].val);
	}
	kfree(ctx->var_table);
	kfree(ctx->argv);
	kfree(ctx);
}

EvalCtx *newShellCtx() {
	EvalCtx *ctx = (EvalCtx*)kmalloc(sizeof(EvalCtx));
	if (ctx==NULL) {
		kprintf("Err -> Out of Memory\n");
		return NULL;
	}
	memset((void*)ctx, 0, sizeof(EvalCtx));
	ctx->global=global;
	ctx->var_cap=16;
	ctx->argc=16;
	ctx->var_table = (Var*)kmalloc(sizeof(Var)*ctx->var_cap);
	ctx->argv = (Var*)kmalloc(sizeof(Var)*ctx->argc);
	memset(ctx->var_table, 0, ctx->var_cap * sizeof(Var));
	memset(ctx->argv, 0, ctx->argc * sizeof(Var));
	ctx->command_handler = multimodule_handler;
	/*
	 * Continuar
	 */
	if (ctx->var_table==NULL && ctx->var_cap!=0) {
		kprintf("Err -> Out of Memory\n");
		kfree(ctx);
		return NULL;
	}

	ctx->last_exit_code=0;
	ctx->should_break=false;
	ctx->should_ret=false;
	ctx->ret = (ShellValue){.type = VAL_NONE};
	ctx->should_exit=false;
	ctx->arg_used = 0;
	ctx->parent = NULL;

	int exit_code = EVAL_OK;
	return ctx;
}

void killShellCtx(EvalCtx *ctx) {
	if (ctx==NULL) return;

	for (size_t i=0;i<ctx->var_cap;i++) {
		if (ctx->var_table[i].is_set) {
			ref_del(*(ShellValue*)((size_t)ctx->var_table[i].name-sizeof(ValType)));
			ref_del(ctx->var_table[i].val);
		}
	}
	for (size_t i=0;i<ctx->arg_used;i++) {
		ref_del(ctx->argv[i].val);
	}
	kfree(ctx->var_table);
	kfree(ctx->argv);
	kfree(ctx);
}

void delShellVarEntry(EvalCtx *ctx, char *name) {	
	size_t id=0;
	size_t s_val = zhash(name) % ctx->var_cap;
	bool done = false;
	if (ctx->var_table[s_val].name != NULL) {
		if (strcmp(ctx->var_table[s_val].name, name)==0) {
			id = s_val;
			done = true;
		}
	}
	for (size_t i=0;i<ctx->var_cap;i++) {	
		if (done) break;
		if (ctx->var_table[i].name==NULL) continue;
		if (strcmp(name, ctx->var_table[i].name)==0) {
			id=i;
			break;
		}
	}
	if (ctx->var_cap <= id) {
		kprintf("Err -> Id exceeds the table capacity\n");
		return;
	}
	ref_del(ctx->var_table[id].val);
	ctx->var_table[id].name = NULL; // solo puedo hacerlo aqui, AHAM
}

Var *getShellVarEntry(EvalCtx *ctx, char *name) {
	if (name==NULL) return NULL;
	size_t id=0;
	size_t s_val = zhash(name) % ctx->var_cap;
	bool done = false;
	if (ctx->var_table[s_val].name != NULL) {
		if (strcmp(ctx->var_table[s_val].name, name)==0) {
			
			id = s_val;
			done = true;
		}
	}
	for (size_t i=0;i<ctx->var_cap;i++) {		
		if (done) break;
		if (ctx->var_table[i].name==NULL) continue;
		//kprintf("AHAM-> %s\n", ctx->var_table[i].name);
		if (strcmp(name, ctx->var_table[i].name)==0) {
			id=i;
			done=true;
			break;
		}
	}
	if (!done) {
		if (ctx->parent!=NULL) {
			return getShellVarEntry(ctx->parent, name);
		} else if (ctx->global!=NULL) return getShellVarEntry(ctx->global, name);
		kprintf("Err -> Var not defined yet\n");
		return NULL;
	}
	return &ctx->var_table[id];
}

Var *getShellVarEntrySilent(EvalCtx *ctx, char *name) {
	if (name==NULL) return NULL;
	size_t id=0;
	size_t s_val = zhash(name) % ctx->var_cap;
	bool done = false;
	if (ctx->var_table[s_val].name != NULL) {
		if (strcmp(ctx->var_table[s_val].name, name)==0) {
			id = s_val;
			done = true;
		}
	}
	for (size_t i=0;i<ctx->var_cap;i++) {	
		if (done) break;
		if (ctx->var_table[i].name==NULL) continue;
		if (strcmp(name, ctx->var_table[i].name)==0) {
			id=i;
			done=true;
			break;
		}
	}
	if (!done) {
		if (ctx->parent!=NULL) {
			return getShellVarEntrySilent(ctx->parent, name);
		} else if (ctx->global!=NULL) return getShellVarEntrySilent(ctx->global, name);
		return NULL;
	}
	return &ctx->var_table[id];
}
void defShellVarEntry(EvalCtx *ctx, char *name, ShellValue val) {
	if (name==NULL) return;
	size_t id=ctx->var_cap+1;
	size_t fid=ctx->var_cap+1;
	size_t s_val = zhash(name) % ctx->var_cap;
	bool done = false;
	if (!ctx->var_table[s_val].is_set) {
		id = s_val;
		done = true;
	}
	for (size_t i=0;i<ctx->var_cap;i++) {
		if (done) break;
		if (!ctx->var_table[i].is_set) {
			fid=i;
			break;
		}
	}
	
	
	if (id>=ctx->var_cap) {
		id=fid; // x si encontramos espacio libre
	}
	if (id >= ctx->var_cap) {
		size_t new_cap = ctx->var_cap * 2;
		Var *new_table = realloc(ctx->var_table, new_cap*sizeof(Var));
		memset((void*)((size_t)new_table+ctx->var_cap*sizeof(Var)), 0, (new_cap-ctx->var_cap) * sizeof(Var));
		ctx->var_cap=new_cap;
		ctx->var_table = new_table;	
	}
	ref_del(ctx->var_table[id].val);	
	ctx->var_table[id] = (Var){.val=ref_add(val), .is_exported=false, .is_set=true};
	ctx->var_table[id].name = name;
}


void setShellVarEntry(EvalCtx *ctx, char *name, ShellValue val) {
	if (name==NULL) return;
	size_t id=ctx->var_cap+1;
	size_t fid=ctx->var_cap+1;
	size_t s_val = zhash(name) % ctx->var_cap;
	bool done = false;
	if (ctx->var_table[s_val].name != NULL) {
		if (strcmp(ctx->var_table[s_val].name, name)==0) {
			id = s_val;
			done = true;
		}
	}
	for (size_t i=0;i<ctx->var_cap;i++) {
		if (done) break;
		if (!(ctx->var_table[i].name==NULL)) {
			if (strcmp(ctx->var_table[i].name, name)==0) {
				id=i;
				break;
			}
		}
	}
	
	
	if (id>=ctx->var_cap) {
		id=fid; // x si encontramos espacio libre
	}
	if (id >= ctx->var_cap) {
		if (ctx->parent!=NULL) {
			setShellVarEntry(ctx->parent, name, val);
			return;
		}
		kprintf("Err -> Var isn't defined yet!\n");
		return;
	}
	ref_del(ctx->var_table[id].val);
	ctx->var_table[id] = (Var){.val=ref_add(val), .is_exported=false, .is_set=true};
	ctx->var_table[id].name = name;
}

ShellValue ref_add(ShellValue val) {
	if (!isHeap(val.as.comodin)) return val;
	if (val.type==VAL_STRING ||
	    val.type==VAL_FUNC ||
	    val.type==VAL_DICT ||
	    val.type==VAL_ARRAY ||
	    val.type==VAL_OBJ) {
		Obj *data = (Obj*)((size_t)val.as.comodin - sizeof(Obj));
		data->refc++;
	}	
	return val;
}

void ref_del(ShellValue val) {
	if (!isHeap(val.as.comodin)) return;
	if (val.type==VAL_STRING ||
	    val.type==VAL_FUNC ||
	    val.type==VAL_DICT ||
	    val.type==VAL_ARRAY ||
	    val.type==VAL_OBJ) {
		Obj *data = (Obj*)((size_t)val.as.comodin - sizeof(Obj));	
		if (data->refc != 0) {
			data->refc--;
		} else {
			zynk_gc();
		}
	}	
}

void addShellArg(EvalCtx *ctx, ShellValue val) {
	if (ctx->arg_used >= ctx->argc) {
		kprintf("Err -> Too Many Args\n");
return;
	}
	ref_add(val);
	ctx->argv[ctx->arg_used++] = (Var){.val=val, .is_exported=false, .is_set=true};
}

ShellValue getShellArg(EvalCtx *ctx, size_t id) {
	if (id >= ctx->arg_used) {
		kprintf("Err -> Arg n. %d doesn't exist	\n", (int)id);
		return newNumVal(0);
	}
	return ctx->argv[id].val;
}



int eval(ASTNode *ast, EvalCtx *ctx) {
	if (ctx==NULL || ast==NULL) return EVAL_ERR; // fatal error!
	
	defShellVarEntry(ctx, "ret", newNumVal(0));
	
	int exit_code = evalProgram(ast, ctx);
	if (ctx->should_exit) {
		exit_code = ctx->exit_code;
	}
	return exit_code;
}

static int evalProgram(ASTNode *program, EvalCtx *ctx) {
	if (program->type!=NODE_PROGRAM) return EVAL_ERR;
	int cexit_code = EVAL_OK;
	for (size_t i=0;i<program->data.program.size;i++) {
		cycles = (cycles+1)%CYCLES_PER_SWEEP;
		if (cycles==0) {
			zynk_gc();
		}
		cexit_code = evalStatement(program->data.program.stmts[i], ctx);
		if (ctx->should_ret || ctx->should_exit) {
			break;
		}
	}
	ctx->last_exit_code = cexit_code;
	return cexit_code;
}



static int evalStatement(ASTNode *stmt, EvalCtx *ctx) {
	if (stmt==NULL) return EVAL_ERR;
	if (ctx->should_ret) return EVAL_RET;
	if (ctx->should_exit) return EVAL_EXIT;


	switch (stmt->type) {
		case CLASS_DEF:
			return evalClassDef(stmt, ctx);
		case FUNCTION_CALL:
			{
				evalFuncCall(stmt, ctx);
				return 0;
			}
		case FUNCTION_DEF: return evalFuncDef(stmt, ctx);
		case VAR_DEL: return evalVarDel(stmt, ctx);
		case VAR_DEF: return evalVarDef(stmt, ctx);
		case NODE_ASSIGNMENT: return evalAssignment(stmt, ctx);
		case NODE_COMMAND_CALL: return evalCommandCall(stmt, ctx);
		case NODE_IF: return evalIf(stmt, ctx);
		case NODE_WHILE_LOOP: return evalWhile(stmt, ctx);
		case NODE_FOR_LOOP: return evalFor(stmt, ctx);
		case NODE_ECHO: return evalEcho(stmt, ctx);
		case NODE_BREAK: {
			ctx->should_break=true;
			return EVAL_BREAK;
				 }
		case NODE_RETURN: {
			ctx->should_ret = true;
			if (stmt->data.ret_stmt.expr!=NULL) {
				ctx->ret = evalExpr(stmt->data.ret_stmt.expr, ctx);
			} else {
				ctx->ret = newNumVal(0);
			}
			return EVAL_RET;
		default: return EVAL_ERR; // Unknown Node, FATAL ERROR!
				  }
	}
	return EVAL_OK;
}

static int evalFuncDef(ASTNode *stmt, EvalCtx *ctx) {
	ShellValue func = newFuncVal(ctx, stmt->data.fn_def.program);
	defShellVarEntry(ctx, stmt->data.fn_def.name, func);
	return 0;
}

static ShellValue evalFuncCall(ASTNode *stmt, EvalCtx *ctx) {
	char *name;
	EvalCtx *rctx = evalMembers(stmt->data.fn_call.member, stmt->data.fn_call.name, ctx, &name);
	Var *var = getShellVarEntry(rctx, name);
	if (var == NULL) return (ShellValue){.type = VAL_NONE};

	if (var->val.type == VAL_FUNC) {
		ZFunc *func = var->val.as.func;
		EvalCtx *fctx = newShellCtx();
		for (int i=0; i<stmt->data.fn_call.argc; i++) {
			addShellArg(fctx, evalExpr(stmt->data.fn_call.args[i], ctx));
		}
		fctx->parent = func->ctx;
		eval(func->program, fctx);
		ShellValue ret = fctx->ret;
		midKillShellCtx(fctx);
		return ret;
	} else if (var->val.type == VAL_CFUNC) {
		CFunc *func = var->val.as.c_func;
		ShellValue args[stmt->data.fn_call.argc];
		for (int i=0; i<stmt->data.fn_call.argc; i++) {
			args[i]=evalExpr(stmt->data.fn_call.args[i], ctx);
		}

		ShellValue ret = func->func(stmt->data.fn_call.argc, args);
		return ret;
	} else if (var->val.type == VAL_DICT) {
		
		ClassObj *klass = (ClassObj*)((size_t)var->val.as.comodin - sizeof(GCObj));
		
		EvalCtx *instance_ctx = newShellCtx();
		instance_ctx->parent = klass->ctx;

		
		eval(klass->program, instance_ctx);

		
		return newInstanceVal(instance_ctx);
	}

	kprintf("Err -> Call target is not a function or class\n");
	return (ShellValue){.type = VAL_NONE};
}

static EvalCtx *evalMembers(ASTNode *stmt, char *name, EvalCtx *ctx, char **oname) {
	
	if (stmt == NULL) {
		*oname = name;
		return ctx;
	}
	Var *base_var = getShellVarEntrySilent(ctx, name);
	EvalCtx *rctx = ctx;

	if (base_var != NULL && base_var->val.type == VAL_DICT) {
		
		InstanceObj *inst = (InstanceObj*)((size_t)base_var->val.as.comodin - sizeof(GCObj));
		rctx = inst->ctx;
	}

	

	
	ASTNode *current = stmt;
	char *rname = name;

	while (current != NULL) {
		
		ShellValue val = evalExpr(current->data.member.expr, ctx);
		if (val.type == VAL_STRING) {
			rname = ref_add(val).as.str;
		}

		
		if (current->data.member.member != NULL) {
			Var *m_var = getShellVarEntrySilent(rctx, rname);
			if (m_var != NULL && m_var->val.type == VAL_DICT) {
				InstanceObj *inst = (InstanceObj*)((size_t)m_var->val.as.comodin - sizeof(GCObj));
				rctx = inst->ctx;
			}
			current = current->data.member.member;
		} else {
			Var *m_var = getShellVarEntrySilent(rctx, rname);
            		if (m_var != NULL && m_var->val.type == VAL_DICT) {
                		InstanceObj *inst = (InstanceObj*)((size_t)m_var->val.as.comodin - sizeof(GCObj));
                		rctx = inst->ctx;
            		}
			break;
		}
	}

	*oname = rname;
	return rctx;
}

static int evalVarDel(ASTNode *stmt, EvalCtx *ctx) {
	char *name;
	EvalCtx *rctx = evalMembers(stmt->data.var_del.member, stmt->data.var_del.name, ctx, &name);
	delShellVarEntry(rctx, name);
	return 0;
}

static int evalVarDef(ASTNode *stmt, EvalCtx *ctx) {
	ShellValue val = evalExpr(stmt->data.var_def.expr, ctx);
	char *name;
	EvalCtx *rctx = evalMembers(stmt->data.var_def.member, stmt->data.var_def.var_name, ctx, &name);	
	defShellVarEntry(rctx, name, val);
	return 0;
}

static int evalEcho(ASTNode *stmt, EvalCtx *ctx) {
	ShellValue val = evalExpr(stmt->data.echo_stmt.expr, ctx);
	if (val.type == VAL_NUMBER) {
		kprintf("%f\n", val.as.num);
	} else if (val.type == VAL_STRING) {
		kprintf("%s\n", (const char *)val.as.str);
	} else if (val.type == VAL_DICT) {
		InstanceObj *inst = (InstanceObj*)((size_t)val.as.comodin - sizeof(GCObj));
		EvalCtx *rctx = inst->ctx;	
		for (int i=0;i<rctx->var_cap;i++) {
			if (rctx->var_table[i].name==NULL) continue;
			printf("Name: %s -> ", rctx->var_table[i].name);
			if (rctx->var_table[i].val.type==VAL_STRING) {
				printf("%s\n", rctx->var_table[i].val.as.str);
			} else if (rctx->var_table[i].val.type==VAL_NUMBER){
				printf("%lf\n", rctx->var_table[i].val.as.num);
			}
		}
	} else {
		kprintf("Err -> Unknown type\n");
	}
	return EVAL_OK;
}

static int evalAssignment(ASTNode *stmt, EvalCtx *ctx) {
	char *name;
	EvalCtx *rctx = evalMembers(stmt->data.assignment.member, stmt->data.assignment.var_name, ctx, &name);
	ShellValue val = evalExpr(stmt->data.assignment.expr, ctx);
	setShellVarEntry(rctx, name, val);
	return EVAL_OK;
}

bool isTruly(ShellValue val) {
	if (val.type == VAL_NUMBER) {
		return (val.as.num != 0);
	} else if (val.type == VAL_STRING) {
		return (val.as.str != NULL && strlen(val.as.str) > 0);
	} else {
		return false;
	}
}

static int evalIf(ASTNode *stmt, EvalCtx *ctx) {
	ShellValue cond_val = evalExpr(stmt->data.if_stmt.condition, ctx);

	bool is_true = isTruly(cond_val);
	ref_del(cond_val);

	int status = EVAL_OK;
	if (is_true) {
		for (size_t i=0;i<stmt->data.if_stmt.s_then;i++) {
			status = evalStatement(stmt->data.if_stmt.thenb[i], ctx);
			if (status == EVAL_BREAK || status == EVAL_ERR || status == EVAL_RET || status == EVAL_EXIT) return status;
		}
	} else if (stmt->data.if_stmt.elseb!=NULL) {
		for (size_t i=0;i<stmt->data.if_stmt.s_else;i++) {
			status = evalStatement(stmt->data.if_stmt.elseb[i], ctx);
			if (status==EVAL_BREAK || status==EVAL_RET || status == EVAL_EXIT || status == EVAL_ERR) return status;
		}
	}
	return status;
}

static int evalWhile(ASTNode *stmt, EvalCtx *ctx) {
	int status = EVAL_OK;
	do {
		for (size_t i=0;i<stmt->data.while_loop.s_body;i++) {
			
			status = evalStatement(stmt->data.while_loop.body[i], ctx);
			if (status == EVAL_BREAK) {
				ctx->should_break = false;
				return EVAL_OK;
			}
			if (status == EVAL_ERR || status == EVAL_EXIT || status == EVAL_RET) return status;
		}

		ShellValue cond_val=evalExpr(stmt->data.while_loop.condition, ctx);
		bool keep = isTruly(cond_val);
		ref_del(cond_val);
		if (!keep) break;
	} while (true);
	return EVAL_OK;
}

static int evalFor(ASTNode *stmt, EvalCtx *ctx) {
	int status=EVAL_OK;

	if (stmt->data.for_loop.init) {
		evalStatement(stmt->data.for_loop.init, ctx);
	}
	while (true) {
		bool keep=false;
		
		if (stmt->data.for_loop.condition) {
			ShellValue cond_val = evalExpr(stmt->data.for_loop.condition, ctx);
			keep = isTruly(cond_val);
			ref_del(cond_val);
		}
		if (!keep) break;

		for (size_t i=0;i<stmt->data.for_loop.s_body;i++) {
			status = evalStatement(stmt->data.for_loop.body[i], ctx);
			if (status == EVAL_BREAK) {
				ctx->should_break=false;
				return EVAL_OK;
			}
			if (status != EVAL_OK) return status; // me di cuenta de que esto es más rápido, xD
		}
		if (stmt->data.for_loop.inc) {
			evalStatement(stmt->data.for_loop.inc, ctx);
		}
	}
	return EVAL_OK;
}

ShellValue evalExpr(ASTNode *expr, EvalCtx *ctx) {
	if (!expr || !ctx) {
		kprintf("Err -> Invalid parameters\n");
		return (ShellValue){.type=VAL_NONE};
	}
	switch (expr->type) {
		case FUNCTION_CALL: return evalFuncCall(expr, ctx);
		case NODE_NUM_LIT: return evalNum(expr);
		case NODE_STRING_LIT: return evalStr(expr);
		case NODE_VAR_REF: return evalVarRef(expr, ctx);
		case NODE_ARG_REF: return evalArgRef(expr, ctx);
		case NODE_BINARY_EXPR: return evalBinExpr(expr, ctx);
		case NODE_UNARY: return evalUnaryExpr(expr, ctx);
		default: kprintf("Err -> Unknown Op\n"); return (ShellValue){.type=VAL_NONE};
	}
}

static ShellValue evalNum(ASTNode *num) {
	return newNumVal(num->data.number_lit);
}

static ShellValue evalStr(ASTNode *str) {
	return (ShellValue){.type=VAL_STRING, .as.str=str->data.string_lit};
}

static ShellValue evalVarRef(ASTNode *ref, EvalCtx *ctx) {
	char *name;
	EvalCtx *rctx = evalMembers(ref->data.var_ref.member, ref->data.var_ref.name, ctx, &name);
	Var *var_entry = getShellVarEntry(rctx, name);	
	if (var_entry==NULL) {
		return (ShellValue){.type=VAL_NONE};
	}
	return var_entry->val;
}
// TODO: hacer q los argumentos acepten miembros
static ShellValue evalArgRef(ASTNode *ref, EvalCtx *ctx) {
	return ref_add(getShellArg(ctx, ref->data.arg_ref.num));
}

static ShellValue evalUnaryExpr(ASTNode *expr, EvalCtx *ctx) {
	ShellValue expected = evalExpr(expr->data.unary_expr.expr, ctx); // no se porque se llama expected
	if (expected.type==VAL_NONE) {
		return expected;
	}
	ShellValue result;
	switch (expr->data.unary_expr.op) {
		case TOKEN_MINUS: {
			if (expected.type == VAL_NUMBER) {
				result = newNumVal(-expected.as.num);
			} else {
				return expected;
			}
			break;
				  }
		case TOKEN_BANG: {
			result = (ShellValue){.type=VAL_NUMBER, .as.num=(long)(!isTruly(expected))};
			break;
				 }
		default: return expected;
	}
	ref_del(expected);
	return result;
}

static ShellValue evalBinExpr(ASTNode *expr, EvalCtx *ctx) {
	ShellValue left = evalExpr(expr->data.binary_expr.left, ctx);
	if (left.type==VAL_NONE) {
		return left; // por pereza a crear un nuevo objeto
	}
	ShellValue right = evalExpr(expr->data.binary_expr.right, ctx);
	if (right.type==VAL_NONE) {
		return right; // por pereza
	}

	ShellValue result;

	bool is_math = (expr->data.binary_expr.op == TOKEN_PLUS ||
                             expr->data.binary_expr.op == TOKEN_MINUS ||
                             expr->data.binary_expr.op == TOKEN_MUL ||
                             expr->data.binary_expr.op == TOKEN_DIV);
	bool is_cmp = (expr->data.binary_expr.op == TOKEN_EQ ||
			     expr->data.binary_expr.op == TOKEN_AND ||
			     expr->data.binary_expr.op == TOKEN_OR ||
                             expr->data.binary_expr.op == TOKEN_GREATER ||
                             expr->data.binary_expr.op == TOKEN_LESS ||
                             expr->data.binary_expr.op == TOKEN_LEQ ||
                             expr->data.binary_expr.op == TOKEN_GEQ ||
                             expr->data.binary_expr.op == TOKEN_BEQ);

	if (is_math) {
		/*
		 * En el futuro mejorare esto, digo, añadire coerciones
		 * no se
		 *
		 */
		if (left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
			switch (expr->data.binary_expr.op) {
				case TOKEN_MINUS: result = newNumVal(left.as.num - right.as.num); break;
				case TOKEN_MUL: result = newNumVal(left.as.num * right.as.num); break;
				case TOKEN_DIV: {
					if (right.as.num==0) {
						result = newNumVal(0);
					} else {
						result = newNumVal(left.as.num / right.as.num);
					}
					break;
						}
				case TOKEN_PLUS: result = newNumVal(left.as.num + right.as.num); break;
			}
		} else if (left.type == VAL_STRING || right.type == VAL_STRING) {
			if (expr->data.binary_expr.op==TOKEN_PLUS) {
				char *right_str;
				char *left_str;
				char num_buff[512];
				if (left.type == VAL_STRING) {
					left_str = left.as.str;
				} else {
					snprintf(num_buff, 512, "%lf", left.as.num);
					left_str = num_buff;
	
				} if (right.type == VAL_STRING) {
					right_str = right.as.str;
				} else {
					snprintf(num_buff, 512, "%lf", right.as.num);
					right_str = num_buff;
				}
				char *concat = (char*)kmalloc(strlen(left_str)+strlen(right_str)+1);
				if (concat==NULL) {
					kprintf("[FATAL ERROR]\n");
					return result;
				}
				strcpy(concat, left_str);
				strcat(concat, right_str);
	
				result = newStrVal(concat);
				kfree(concat);
			}
		}
	} else if (is_cmp) {
		long num_left=0, num_right=0;
		char *str_left=NULL, *str_right=NULL;
		bool can_num=false;
		bool can_str=false;

		if (left.type==VAL_NUMBER) {
			num_left = left.as.num;
			can_num=true;
		} else if (left.type==VAL_STRING && left.as.str!=NULL) {
			str_left = left.as.str;
			can_str = true;
		} else {
			kprintf("Err -> Illegal type\n");
			result = newNumVal(0);
		}
		if (right.type==VAL_NUMBER) {
			num_right = right.as.num;
			can_num = can_num && true;
		} else if (right.type==VAL_STRING && right.as.str!=NULL) {
			str_right = right.as.str;
			can_str = can_str && true;
		} else {
			kprintf("Err -> Illegal type\n");
			result = newNumVal(0);
		}

		if (can_num) {
			switch (expr->data.binary_expr.op) {
				case TOKEN_OR: result=newNumVal(num_left || num_right); break;
				case TOKEN_AND: result=newNumVal(num_left && num_right); break;
				case TOKEN_EQ: result=newNumVal(num_left==num_right); break;
				case TOKEN_GREATER: result=newNumVal(num_left>num_right); break;
				case TOKEN_LESS: result=newNumVal(num_left < num_right); break;
				case TOKEN_LEQ: result=newNumVal(num_left <= num_right); break;
				case TOKEN_GEQ: result=newNumVal(num_left >= num_right); break;
				case TOKEN_BEQ: result=newNumVal(num_left != num_right); break;
			}
		} else if (can_str) {
			int cmp = strcmp(str_left, str_right);
			switch (expr->data.binary_expr.op) {
				case TOKEN_EQ: result=newNumVal(cmp==0); break;
				case TOKEN_BEQ: result=newNumVal(cmp!=0); break;
				case TOKEN_LEQ: result=newNumVal(cmp<=0); break;
				case TOKEN_GEQ: result=newNumVal(cmp>=0); break;
				case TOKEN_LESS: result=newNumVal(cmp<0); break;
				case TOKEN_GREATER: result=newNumVal(cmp>0); break;
			}
		} else {
			kprintf("Err -> Unknown Operation\n");
			result = (ShellValue){.type=VAL_NONE};
		}
	}

	return result;
}

static int evalCommandCall(ASTNode *stmt, EvalCtx *ctx) {
	if (ctx->command_handler == NULL) {
		return EVAL_OK; // si no hay nada, devolvemos ok y a llorar
	} else {
		return ctx->command_handler(stmt, ctx);
	}
}
