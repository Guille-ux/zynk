#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "mbtype.h"
#include "common.h"

typedef enum {
	NODE_PROGRAM,
	NODE_ASSIGNMENT,
	NODE_COMMAND_CALL,
	NODE_EXPORT,
	NODE_VAR_REF,
	NODE_ARG_REF,
	NODE_STRING_LIT,
	NODE_NUM_LIT,
	NODE_BINARY_EXPR,
	NODE_IF,
	NODE_WHILE_LOOP,
	NODE_FOR_LOOP,
	NODE_BREAK,
	NODE_ECHO,
	NODE_RETURN,
	NODE_UNARY,
	CLASS_DEF,
	FUNCTION_DEF,
	FUNCTION_CALL,
	VAR_DEF,
	VAR_DEL,
	MEMBER_ACCESS,
} ASTNodeType;

struct ASTNode;
struct ProgramNode;
struct AssignmentNode;
struct CommandCallNode;
struct VarRefNode;
struct ExportNode;
struct BinExprNode;
struct IfNode;
struct WhileNode;
struct ForNode;
struct EchoNode;
struct RetNode;
struct UnaryNode;
struct ArgRefNode;
struct FnNode;
struct CallNode;
struct ClassNode;
struct VarDefNode;
struct VarDelNode;
struct MemberNode;

typedef struct ProgramNode {
	struct ASTNode **stmts;
	size_t size;
} ProgramNode;

typedef struct ArgRefNode {
	long num;
	struct ASTNode *member;
} ArgRefNode;

typedef struct AssignmentNode {
	char *var_name;
	struct ASTNode *member;
	struct ASTNode *expr;
} AssignmentNode;

typedef struct VarDefNode {
	char *var_name;
	struct ASTNode *member;
	struct ASTNode *expr;
} VarDefNode;

typedef struct CommandCallNode {
	char *name;
	struct ASTNode **args;
	size_t argc;
} CommandCallNode;

typedef struct VarRefNode {
	char *name;
	struct ASTNode *member;
} VarRefNode;

typedef struct VarDelNode {
	char *name;
	struct ASTNode *member;
} VarDelNode;

typedef struct BinExprNode {
	LTokenType op;
	struct ASTNode *left;
	struct ASTNode *right;
} BinExprNode;

typedef struct IfNode {
	struct ASTNode *condition;
	struct ASTNode **thenb;
	struct ASTNode **elseb;
	size_t s_then;
	size_t s_else;
} IfNode;

typedef struct WhileNode {
	struct ASTNode *condition;
	struct ASTNode **body;
	size_t s_body;
} WhileNode;

typedef struct ForNode {
	struct ASTNode *init;
	struct ASTNode *condition;
	struct ASTNode *inc;
	struct ASTNode **body;
	size_t s_body;
} ForNode;

typedef struct EchoNode {
	struct ASTNode *expr;
} EchoNode;

typedef struct RetNode {
	struct ASTNode *expr;
} RetNode;

typedef struct UnaryNode {
	LTokenType op;
	struct ASTNode *expr;
} UnaryNode;

typedef struct ExportNode {
	struct ASTNode *var; // no es el nombre de la variable porque
			     // tambien puede ser una asignación
			     // Aunque no todavia
} ExportNode;

typedef struct MemberNode {
	struct ASTNode *expr;
	struct ASTNode *member;
} MemberNode;


typedef struct FnNode {
	char *name;
	struct ASTNode *program;
} FnNode;

typedef struct CallNode {
	char *name;
	struct ASTNode *member;
	struct ASTNode **args;
	size_t argc;
} CallNode;

typedef struct ClassDef {
	char *name;
	struct ASTNode *program;
} ClassDef;



typedef struct ASTNode {
	ASTNodeType type;
	union {
		ProgramNode program;
		AssignmentNode assignment;
		CommandCallNode command_call;
		VarRefNode var_ref;
		BinExprNode binary_expr;
		IfNode if_stmt;
		WhileNode while_loop;
		ForNode for_loop;
		EchoNode echo_stmt;
		RetNode ret_stmt;
		UnaryNode unary_expr;
		ExportNode export_stmt;
		ArgRefNode arg_ref;
		VarDefNode var_def;
		VarDelNode var_del;
		MemberNode member;
		FnNode fn_def;
		CallNode fn_call;
		ClassDef class_def;

		char *string_lit;
		double number_lit;
	} data;
} ASTNode;

void parser_init(Token *token_list);
ASTNode *parse();
void parser_free_ast(ASTNode *node);

#endif
