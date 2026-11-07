#include "../include/capi.h"

int initGlobals() {
	register_globals(get_global());

	return 0;
}

int init() {
	init_tracker();
	EvalCtx *global = newShellCtx();
	set_global(global);
	return 0;
}

int killZynk() {
	killShellCtx(get_global());
	zynk_clean();
	cleanAll();
	return 0;
}

ShellValue exec(const char *code, int argc, ShellValue *argv) {
	EvalCtx *ctx = newShellCtx();

	addShellArg(ctx, newNumVal((double)argc));
	for (int i=0;i<argc;i++) {
		addShellArg(ctx, argv[i]);
	} // im sorry but if i added it to the globals 
	  // we'll have the thing full of shit
	  // so yeah, if you import something it can't use your
	  // args, i'm sorry

	
	lexer_init(code);

	Token *t_buff = lexe(NULL); // TOKENINZING THE CODE!!!

	parser_init(t_buff);

	ASTNode *tree = parse();

#ifndef BYTECODE_YET
	eval(tree, ctx);
#else 
	compile(tree);
	initVM(&vm);
	loadChunk(&vm, &chk);
	//disassembleChunk(&chk, "CHUNK");
	run(&vm);
	// idk
	freeVM(&vm);
#endif
	
	parser_free_ast(tree);
	lexer_free(t_buff);
	free(t_buff);

	ShellValue val = ref_add(ctx->ret);

	killShellCtx(ctx);

	cleanAll();

	return val; // adding references, programs have to
			  // delete the reference
}
