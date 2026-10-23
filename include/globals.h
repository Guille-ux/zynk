#ifndef ZYNK_GLOBALS_H
#define ZYNK_GLOBALS_H

#include "eval.h"

#define MAX_NATIVE_FUNCS 64


void cleanAll();
int load_plugins(const char *file, EvalCtx *ctx);


void register_cfunc(EvalCtx *ctx, char *name, NativeFunc func);
void register_globals(EvalCtx *ctx);


// funciones a registrar
ShellValue len(int argc, ShellValue *args);
ShellValue zindex(int argc, ShellValue *args);


#endif
