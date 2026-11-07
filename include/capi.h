#ifndef ZYNK_C_API_H
#define ZYNK_C_API_H

#include "eval.h"
#include "globals.h"
#include "parser.h"
#include "lexer.h"

/*
 *
 * This should make easir for idk, programs in C interact with Zynk
 *
 *
 */

int killZynk();
int cleanZynk();

int init(); // initializes the memory tracker, the global eval ctx
	    // etc etc
int initGlobals(); // just initializes the globals from std zynk yk
		   // if you wanna add more you'll have to add other
		   // things
ShellValue exec(const char *code, int argc, ShellValue *argv);

#endif
