#include <stdio.h>
#include <stdint.h>
#include "../include/eval.h"
#include "../include/parser.h"
#include "../include/lexer.h"
#include "../include/modules.h"
#include "../include/globals.h"
#include "../include/compiler.h"
#include "../include/vm.h"
#include "../include/payload.h"

int main(int argc, char *argv[]) {
	//initArena();	
	EvalCtx *global = newShellCtx();

	set_global(global);
	initFuncStorage();
	register_globals(global);
	addShellArg(global, newNumVal((double)argc));
	for (int i=0;i<argc;i++) {
		addShellArg(global, newStrVal(argv[i]));
	}

	


	


	



	EvalCtx *global_env = newShellCtx();
	global_env->command_handler = multimodule_handler;
	
	size_t offset = 0;



	char *real_buff = (char*)((size_t)payload + offset);
	lexer_init(real_buff);
	Token *t_buff = lexe(NULL);

	parser_init(t_buff);
	ASTNode *tree = parse();

#ifndef BYTECODE_YET
	eval(tree, global_env);
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
	killShellCtx(global_env);

	cleanAll();
	cleanFuncStorage();
	//afree();
	return 0;
}


