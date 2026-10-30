#include <stdio.h>
#include <stdint.h>
#include "../include/eval.h"
#include "../include/parser.h"
#include "../include/lexer.h"
#include "../include/modules.h"
#include "../include/globals.h"
#include "../include/compiler.h"
#include "../include/vm.h"

int main(int argc, char *argv[]) {
	//initArena();
	if (argc < 2) {
		printf("Use: %s <filename>\n", argv[0]);
		return 1;
	}
	EvalCtx *global = newShellCtx();

	set_global(global);

	register_globals(global);
	addShellArg(global, newNumVal((double)argc));
	for (int i=0;i<argc;i++) {
		addShellArg(global, newStrVal(argv[i]));
	}

	FILE *archivo = fopen(argv[1], "r");


	if (archivo == NULL) {
		perror("Error while reading the file!");
		return 1;
	}

	fseek(archivo, 0, SEEK_END);
	long size = ftell(archivo);
	fseek(archivo, 0, SEEK_SET);  
	
	char *buffer = malloc(size);

	if (buffer==NULL) {
		fclose(archivo);
		return -1;
	}
	size_t read_ = fread(buffer, 1, size, archivo);
	buffer[read_-1] = '\0';
	EvalCtx *global_env = newShellCtx();
	global_env->command_handler = multimodule_handler;

	lexer_init(buffer);
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

	free(buffer);
	fclose(archivo);
	cleanAll();
	//afree();
	return 0;
}
