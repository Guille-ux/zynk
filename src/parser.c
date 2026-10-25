/*
 * parser.c
 * sljaknsnak
 * es mi parser para zynk
 * 
 *
 *
 * No Se - Guillermo Leira Temes 2026 basado en el de mbash
 *
 */

#include "../include/parser.h"
#include "../include/lexer.h"
#include "../include/mbtype.h"

static Token *tokens;
static size_t token_index;

static ASTNode *parse_fn_call();
static ASTNode *parse_fn_def();
static ASTNode *parse_member();
static ASTNode *parse_var_del();
static ASTNode *parse_var_def();
static ASTNode *parse_program();
static ASTNode *parse_statement();
static ASTNode *parse_assignment();
static ASTNode *parse_command_call();
static ASTNode *parse_if();
static ASTNode *parse_while();
static ASTNode *parse_for();
static ASTNode *parse_break();
static ASTNode *parse_echo();
static ASTNode *parse_return();
static ASTNode *parse_expression();
static ASTNode *parse_unary_expr();
static ASTNode *parse_primary_expr();
static ASTNode *parse_class_def();
static ASTNode **parse_statement_list(size_t *out_s, LTokenType *stop_tokens, size_t n_stop_tokens);
static ASTNode **parse_expression_list(size_t *argc);

static Token *peek() {
	return &tokens[token_index];
}

static Token *seeNext() {
	return &tokens[token_index+1];
}

static Token *eat() {
	if (tokens[token_index].type==TOKEN_EOF) return &tokens[token_index];
	return &tokens[token_index++];
}

static bool match(LTokenType type) {
	if (peek()->type != type) return false;
	token_index++;
	return true;
}

static ASTNode *newASTNode(ASTNodeType type) {
	ASTNode *node = (ASTNode *)kmalloc(sizeof(ASTNode));
	if (node == NULL) {
		kprintf("Err -> Out Of Memory!\n");
		return NULL;
	}
	node->type=type;
	memset(&node->data, 0, sizeof(node->data));
	return node;
}

static ASTNode *parse_program() {
	ASTNode *programNode=newASTNode(NODE_PROGRAM);
	if (programNode==NULL) return NULL;

	ProgramNode *program_data=&programNode->data.program;

	program_data->stmts=NULL;
	program_data->size=0;

	size_t cap=10;

	program_data->stmts=(ASTNode **)kmalloc(cap*sizeof(ASTNode*));

	if (!program_data->stmts) {
		kfree(programNode);
		return NULL;
	}

	while (peek()->type != TOKEN_EOF &&
		   peek()->type != TOKEN_RBRACE) {
		if (peek()->type == TOKEN_NEW_LINE) {
			eat();
			continue;
		}

		ASTNode *stmt = parse_statement();
		if (stmt==NULL) {
			parser_free_ast(programNode);
			return NULL;
		}

		if (program_data->size >= cap) {
			cap *= 2;
			ASTNode **new_stmts=(ASTNode**)kmalloc(cap*sizeof(ASTNode*));
			if (!new_stmts) {
				kfree(programNode->data.program.stmts);
				kfree(programNode);
			}

			memcpy(new_stmts, program_data->stmts, program_data->size * sizeof(ASTNode*));

			kfree(program_data->stmts);
			program_data->stmts = new_stmts;
		}

		program_data->stmts[program_data->size++] = stmt;

		if (peek()->type != TOKEN_EOF && (peek()->type == TOKEN_NEW_LINE)) {
			eat();
		}

	}
	return programNode;
}

static ASTNode *parse_member() {
	ASTNode *new_node = newASTNode(MEMBER_ACCESS);
	ASTNode *expr;
	if (peek()->type==TOKEN_LBRACKET) {
		eat();
		expr = parse_expression();
		if (eat()->type!=TOKEN_RBRACKET) {
			kprintf("Err -> Expected Bracket\n");
		}
	} else if (peek()->type==TOKEN_COLON) {
		eat();
		Token *current = eat();
		expr = newASTNode(NODE_STRING_LIT);
		expr->data.string_lit = strdup(current->value);
	} else {
		return NULL; // en plan no hay nada dentro
	}
	new_node->data.member.expr = expr;
	new_node->data.member.member = parse_member();
	return new_node;
}

static ASTNode *parse_identifier() {
    Token *current = eat(); // Nos comemos el TOKEN_IDENTIFIER
    if (current->type != TOKEN_IDENTIFIER) {
        kprintf("Err -> Expected Identifier in parse_identifier\n");
        return NULL;
    }

    char *name = strdup(current->value);
    ASTNode *member = parse_member(); 

    
    if (peek()->type == TOKEN_ASSIGN) {
        eat(); 
        ASTNode *expr = parse_expression();
        if (expr == NULL) {
            kfree(name);
            parser_free_ast(member);
            return NULL;
        }
        ASTNode *node = newASTNode(NODE_ASSIGNMENT);
        node->data.assignment.var_name = name;
        node->data.assignment.member = member;
        node->data.assignment.expr = expr;
        return node;
    }

    
    if (peek()->type == TOKEN_LPAREN) {
        ASTNode *node = parse_fn_call();
        if (node == NULL) {
            kfree(name);
            parser_free_ast(member);
            return NULL;
        }
        node->data.fn_call.name = name;
        node->data.fn_call.member = member;
        return node;
    }

    
    ASTNode *node = newASTNode(NODE_COMMAND_CALL);
    node->data.command_call.name = name;
    
    size_t argc = 0;
    ASTNode **argv = parse_expression_list(&argc);
    node->data.command_call.argc = argc;
    node->data.command_call.args = argv;

    
    if (member != NULL) {
        parser_free_ast(member);
    }

    return node;
}

static ASTNode *parse_statement() {	
	Token *current = peek();
	if (current->type==TOKEN_FN) {
		return parse_fn_def();
	} else if (current->type==TOKEN_CLASS) {
		return parse_class_def();
	} else if (current->type==TOKEN_DEL) {
		return parse_var_del();
	} else if (current->type==TOKEN_IDENTIFIER) {
		return parse_identifier();
		return parse_command_call();
	} else if (current->type==TOKEN_LET) {
		return parse_var_def();
	} else if (current->type==TOKEN_NUMBER) {
		return parse_assignment();
	} else if (current->type == TOKEN_IF) {
		return parse_if();
	} else if (current->type == TOKEN_DO) {
		return parse_while();
	} else if (current->type == TOKEN_FOR) {
		return parse_for();
	} else if (current->type == TOKEN_BREAK) {
		return parse_break();
	} else if (current->type == TOKEN_RETURN) {
		return parse_return();
	} else if (current->type == TOKEN_ECHO) {
		return parse_echo();
	}

	/*
 	 * Supongo que añadire más tipos de nodos, los pipes por ejemplo
 	 * estaria bien añadirlos
 	 *
 	 *
 	 */



	else {
		kprintf("Err -> Unknown Statement!\n");
		kprintf("Token -> %s\n", current->value);
		return NULL;
	}
}

void parser_init(Token *token_list) {
	tokens = token_list;
	token_index=0;
}

ASTNode *parse() {
	return parse_program();
}

void parser_free_ast(ASTNode *node) {
	if (node==NULL) return;

	switch (node->type) {
		case NODE_PROGRAM: {
					for (size_t i=0;i<node->data.program.size;i++) {
						parser_free_ast(node->data.program.stmts[i]);
					}
					kfree(node->data.program.stmts);
					break;
				   }
		case NODE_ASSIGNMENT: {
					kfree(node->data.assignment.var_name);
					parser_free_ast(node->data.assignment.expr);
					parser_free_ast(node->data.assignment.member);
					break;
				      }
		case NODE_COMMAND_CALL: {
					kfree(node->data.command_call.name);
					for (size_t i=0;i<node->data.command_call.argc;i++) {
						parser_free_ast(node->data.command_call.args[i]);
					}
					kfree(node->data.command_call.args);
					break;
					}
		case NODE_VAR_REF: {
					parser_free_ast(node->data.var_ref.member);
					break;
				   }
		case NODE_STRING_LIT: {
					kfree(node->data.string_lit);
					break;
				      }
		case NODE_BINARY_EXPR: {
					parser_free_ast(node->data.binary_expr.left);
					parser_free_ast(node->data.binary_expr.right);
					break;
				       }
		case NODE_UNARY: {
					parser_free_ast(node->data.unary_expr.expr);
					break;
				 }
		case NODE_IF: {
	            			parser_free_ast(node->data.if_stmt.condition);
					for (size_t i = 0; i < node->data.if_stmt.s_then; i++) {
						parser_free_ast(node->data.if_stmt.thenb[i]);
					}
					kfree(node->data.if_stmt.thenb);
					for (size_t i = 0; i < node->data.if_stmt.s_else; i++) {
						parser_free_ast(node->data.if_stmt.elseb[i]);
					}
					kfree(node->data.if_stmt.elseb);
					break;
				}
		case NODE_WHILE_LOOP: {
					parser_free_ast(node->data.while_loop.condition);
					for (size_t i=0;i<node->data.while_loop.s_body; i++) {
						parser_free_ast(node->data.while_loop.body[i]);
					}
					kfree(node->data.while_loop.body);
					break;
				      }
		case NODE_FOR_LOOP: {
					parser_free_ast(node->data.for_loop.init);
					parser_free_ast(node->data.for_loop.condition);
					parser_free_ast(node->data.for_loop.inc);

					for (size_t i=0;i<node->data.for_loop.s_body;i++) {
						parser_free_ast(node->data.for_loop.body[i]);
					}
					kfree(node->data.for_loop.body);
					break;
				    }
		case NODE_ECHO: {
					parser_free_ast(node->data.echo_stmt.expr);
					break;
				}
		case NODE_EXPORT: {
					parser_free_ast(node->data.export_stmt.var);
					break;
				  }
		case NODE_RETURN: {
					parser_free_ast(node->data.ret_stmt.expr);
					break;
				  }
		case CLASS_DEF:
			parser_free_ast(node->data.class_def.program);
			break;
		case FUNCTION_DEF:
			parser_free_ast(node->data.fn_def.program);
			break;
		case FUNCTION_CALL:
			parser_free_ast(node->data.fn_call.member);
			for (int i=0;i<node->data.fn_call.argc;i++) {
				parser_free_ast(node->data.fn_call.args[i]);
			}
			kfree(node->data.fn_call.args);
			break;
		case VAR_DEF: {

			parser_free_ast(node->data.var_def.member);
			parser_free_ast(node->data.var_def.expr);
			break;
			      }
		case VAR_DEL:

			parser_free_ast(node->data.var_del.member);
			break;
		case NODE_ARG_REF:
			parser_free_ast(node->data.arg_ref.member);
			break;
		case MEMBER_ACCESS:
			parser_free_ast(node->data.member.expr);
			parser_free_ast(node->data.member.member);
			break;
		default: break;
	}
	kfree(node);
}

static ASTNode *parse_primary_expr() {
	/*
	 * Empezando!!!
	 *
	 */
	ASTNode *new_node;
	Token *current = peek();
	switch (current->type) {
		case TOKEN_NUMBER: {
			eat();
			new_node = newASTNode(NODE_NUM_LIT);
			new_node->data.number_lit = atof(current->value); // un pelin peligroso
			break;
				   }
		case TOKEN_STRING: {
			eat();
			new_node = newASTNode(NODE_STRING_LIT);
			new_node->data.string_lit = strdup(current->value); // peligroso
			break;
				   }
		case TOKEN_IDENTIFIER: {
			eat();
			ASTNode *member = parse_member();
			char *name = current->value;
			if (peek()->type==TOKEN_LPAREN) {
				new_node = parse_fn_call();
				new_node->data.fn_call.member = member;
				new_node->data.fn_call.name = name;
			} else {
				new_node = newASTNode(NODE_VAR_REF);
				new_node->data.var_ref.member = member;
				new_node->data.var_ref.name = name;
			}
			break;
					}
		case TOKEN_DOLLAR: {
			eat();
			current = peek();
			if (!match(TOKEN_NUMBER)) return NULL;
			ASTNode *member = parse_member();
			size_t number = atol(current->value);
			new_node=newASTNode(NODE_ARG_REF);
			new_node->data.arg_ref.num=number;
			new_node->data.arg_ref.member=member;
			eat();
			break;
				   }
		case TOKEN_LPAREN: {
			current = eat();
			new_node = parse_expression();
			if (!match(TOKEN_RPAREN)) {
				kprintf("Err -> Expected ')'!\n");
				return NULL;
			}	
			break;
				   }
		default: {
			kprintf("Err -> Unexpected Token!\n");
			return NULL;
			 }
	}
	//eat();
	return new_node;
}

static ASTNode *parse_class_def() {
	eat();
	ASTNode *new_node = newASTNode(CLASS_DEF);
	Token *name = eat();
	if (name->type!=TOKEN_IDENTIFIER) kprintf("Err -> expected Identifier\n");
	//kprintf("%s\n", name->value);
	if (eat()->type!=TOKEN_LBRACE) kprintf("Err -> '{' expected\n");
	new_node->data.class_def.name = name->value;
	new_node->data.class_def.program = parse_program();
	if (eat()->type!=TOKEN_RBRACE) kprintf("Err -> '}' expected\n");
	return new_node;
}

static ASTNode *parse_fn_def() {
	eat();
	ASTNode *new_node = newASTNode(FUNCTION_DEF);
	Token *name = eat();
	if (name->type!=TOKEN_IDENTIFIER) kprintf("Err -> expected Identifier\n");
	//kprintf("%s\n", name->value);
	if (eat()->type!=TOKEN_LBRACE) kprintf("Err -> '{' expected\n");
	new_node->data.fn_def.name = name->value;
	new_node->data.fn_def.program = parse_program();
	if (eat()->type!=TOKEN_RBRACE) kprintf("Err -> '}' expected\n");
	return new_node;
}

static ASTNode *parse_fn_call() {
	ASTNode *func_node = newASTNode(FUNCTION_CALL);
	eat();  //  nos comemos el token ()
	size_t argc =0;
	func_node->data.fn_call.args = parse_expression_list(&argc);
	func_node->data.fn_call.argc = argc;
	eat(); // comer el '('
	return func_node;
}

static ASTNode *parse_var_del() {
	eat(); // nos comemos del
	ASTNode *new_node = newASTNode(VAR_DEL);
	Token *identifier = eat();
	new_node->data.var_del.name = identifier->value;
	new_node->data.var_del.member = parse_member();
	return new_node;
}

static ASTNode *parse_var_def() {
	eat(); // nos comemos el let
	ASTNode *new_node = newASTNode(VAR_DEF);
	Token *identifier = eat();
	new_node->data.var_def.member = parse_member();
	ASTNode *expr;
	if (peek()->type==TOKEN_ASSIGN) {
		// parseamos la expresión
		eat();
		expr = parse_expression();
		if (expr == NULL) {
			parser_free_ast(new_node);
			return NULL;
		}
	} else {
		expr = newASTNode(NODE_NUM_LIT);
		expr->data.number_lit = 0;
	}
	new_node->data.var_def.var_name = identifier->value;
	new_node->data.var_def.expr = expr;
	return new_node;
}

static ASTNode *parse_unary_expr() {
	Token *current = peek();
	ASTNode *new_node;
	switch (current->type) {
		case TOKEN_MINUS: {
			eat();
			current = peek();
			new_node = newASTNode(NODE_UNARY);
			if (new_node==NULL) return NULL;
			new_node->data.unary_expr.expr = parse_unary_expr();
			new_node->data.unary_expr.op = TOKEN_MINUS;
			break;
				  }
		case TOKEN_BANG: {
			eat();
			current = peek();
			new_node = newASTNode(NODE_UNARY);
			if (new_node==NULL) return NULL;
			new_node->data.unary_expr.expr = parse_unary_expr();
			new_node->data.unary_expr.op = TOKEN_BANG;
			break;
				 }
		default: {
			new_node = parse_primary_expr();
			break;
			 }
	}
	return new_node;
}

/*
 * Definiciones de funciones de la cadena de parseo para expresiones
 *
 */

static ASTNode *parse_factor();
static ASTNode *parse_term();


static ASTNode *parse_factor() {
	return parse_unary_expr();
}

static ASTNode *parse_term() {
	ASTNode *left_node = parse_factor();
	while ((peek()->type == TOKEN_MUL || peek()->type == TOKEN_DIV)) {
		Token *op = eat();

		ASTNode *right_node=parse_factor();
		if (right_node==NULL) {
			kprintf("Err -> error parsing factor\n");
			parser_free_ast(left_node);
			return NULL;
		}
		ASTNode *binExpr = newASTNode(NODE_BINARY_EXPR);
		if (binExpr == NULL) {
			kprintf("Err -> Memory Error\n");
			parser_free_ast(left_node);
			parser_free_ast(right_node);
			return NULL;
		}

		binExpr->data.binary_expr.op=op->type;
		binExpr->data.binary_expr.left=left_node;
		binExpr->data.binary_expr.right=right_node;

		left_node = binExpr;
	}
	return left_node;
}

// Tabla de precedencia de operadores  
static int get_precedence(LTokenType type) {
    switch(type) {
        case TOKEN_MUL: 
        case TOKEN_DIV: 
            return 4;
	case TOKEN_MINUS:
        case TOKEN_PLUS:
            return 3;
        case TOKEN_GREATER:
        case TOKEN_LESS: 
        case TOKEN_GEQ:
        case TOKEN_LEQ:
        case TOKEN_EQ:
        case TOKEN_BEQ: 
            return 2;
	case TOKEN_AND:
	case TOKEN_OR:
	    return 1;
        default: 
            return 0;
    }
}

static ASTNode *parse_binary_expr(int min_precedence) {
    ASTNode *left = parse_unary_expr();
    if(!left) return NULL;

    while(1) {
        Token *op = peek();
        int precedence = get_precedence(op->type);
        
        if(precedence == 0 || precedence < min_precedence) 
            break;

        eat(); // Consume el operador
        
        ASTNode *right = parse_binary_expr(precedence + 1);
        if(!right) {
            parser_free_ast(left);
            return NULL;
        }

        ASTNode *bin_expr = newASTNode(NODE_BINARY_EXPR);
        if(!bin_expr) {
            parser_free_ast(left);
            parser_free_ast(right);
            return NULL;
        }

        bin_expr->data.binary_expr.left = left;
        bin_expr->data.binary_expr.right = right;
        bin_expr->data.binary_expr.op = op->type;
        
        left = bin_expr;
    }
    
    return left;
}

static ASTNode *parse_expression() {
    return parse_binary_expr(0);
}

static ASTNode *parse_assignment() {
	ASTNode *new_node=newASTNode(NODE_ASSIGNMENT);
	if (new_node==NULL) {
		return NULL;
	}	

	Token *name = eat();
	
	if (name->type!=TOKEN_IDENTIFIER) {
		kprintf("Err -> Expected Identifier\n");
		parser_free_ast(new_node);
		return NULL;
	}
	new_node->data.assignment.member = parse_member();
	new_node->data.assignment.var_name = strdup(name->value);
	if (!match(TOKEN_ASSIGN)) {
		kprintf("Err -> Expected '=' after identifier\n");
		parser_free_ast(new_node);
		return NULL;
	}
	ASTNode *expr_node = parse_expression();
	if (expr_node==NULL) {
		kprintf("Err -> error parsing expr\n");
		parser_free_ast(expr_node);
		parser_free_ast(new_node);
		return NULL;
	}
	new_node->data.assignment.expr = expr_node;
	return new_node;
}



static ASTNode *parse_command_call() {
	ASTNode *new_node = newASTNode(NODE_COMMAND_CALL);
	if (new_node==NULL) return NULL;

	Token *cmd = eat();
	if (cmd->type != TOKEN_IDENTIFIER) {
		kprintf("Err -> Expected Identifier but found Token n. %d\n", (int)cmd->type);
		parser_free_ast(new_node);
		return NULL;
	}
	new_node->data.command_call.name = strdup(cmd->value);
	if (new_node->data.command_call.name==NULL && cmd->value != NULL) {
		kprintf("Err -> Out of Memory Error!\n");
		parser_free_ast(new_node);
		return NULL;
	}


	size_t argc=0; // args count
	
	ASTNode **argv = parse_expression_list(&argc);

	if (argv==NULL && argc != 0) {
		kprintf("Err -> Out of Memory\n");
		parser_free_ast(new_node);
		return NULL;
	}
	new_node->data.command_call.argc=argc;
	new_node->data.command_call.args=argv;
	return new_node;
}

static ASTNode **parse_expression_list(size_t *argc) {
	ASTNode **list = NULL;
	size_t size=0;
	size_t cap = 4;

	list = (ASTNode**)kmalloc(sizeof(ASTNode*)*cap);
	if (list==NULL) {
		kprintf("Err -> Out of Memory -\n");
		*argc=0;
		return NULL;
	}

	while  (peek()->type != TOKEN_NEW_LINE &&
		peek()->type != TOKEN_EOF &&
		peek()->type != TOKEN_PIPE &&
		peek()->type != TOKEN_IF &&
		peek()->type != TOKEN_WHILE &&
		peek()->type != TOKEN_FOR &&
		peek()->type != TOKEN_BREAK &&
		peek()->type != TOKEN_RETURN &&
		peek()->type != TOKEN_EXPORT &&
		peek()->type != TOKEN_ECHO &&
		peek()->type != TOKEN_RPAREN) {
		

		ASTNode *arg = parse_expression();
		if (arg == NULL) {
			kprintf("Err -> Out of Memory\n");
			for (size_t i = 0;i<size;i++) {
				parser_free_ast(list[i]);
			}
			kfree(list);
			*argc = 0;
			return NULL;
		}
		
		if (size >= cap) {
			cap *= 2;
			ASTNode **new_list = (ASTNode **)kmalloc(sizeof(ASTNode*)*cap);
			if (new_list == NULL) {
				kprintf("Err -> Out of Memory\n");
				parser_free_ast(arg);
				for (size_t i = 0;i<size;i++) {
					parser_free_ast(list[i]);
				}
				*argc = 0;
				kfree(list);
				return NULL;
			}
			memcpy((void*)new_list, (const void*)list, size * sizeof(ASTNode *));
		
			kfree(list);
			list = new_list;
		}

		list[size++]=arg;
		//if (peek()->type!=TOKEN_COMMA) break;
	}
	*argc = size;
	return list;
}

static ASTNode *parse_echo() {
	ASTNode *new_node = newASTNode(NODE_ECHO);

	if (new_node == NULL) {
		kprintf("Err -> Out of Memory\n");
		return NULL;
	}
	Token *echo_k = eat();
	//lexer_free_token(echo_k);
	ASTNode *expr = parse_expression();
	if (expr == NULL) {
		kfree(new_node);
		return NULL;
	}
	new_node->data.echo_stmt.expr = expr;
	return new_node;
}

static ASTNode *parse_return() {
	ASTNode *new_node = newASTNode(NODE_RETURN);
	if (new_node == NULL) {
		kprintf("Err -> Out of Memory\n");
		return NULL;
	}
	Token *ret_k=eat();
	LTokenType next = peek()->type;
	if     (next != TOKEN_NEW_LINE &&
		next != TOKEN_SEMICOLON &&
		next != TOKEN_COMMA &&
		next != TOKEN_PIPE &&
		next != TOKEN_EOF) {
		

		ASTNode *expr = parse_expression();
		if (expr==NULL) {
			kfree(new_node);
			return NULL;
		}
		new_node->data.ret_stmt.expr=expr;
	} else {
		new_node->data.ret_stmt.expr=NULL;
	}
	return new_node;
}

static ASTNode *parse_break() {
	ASTNode *new_node = newASTNode(NODE_BREAK);
	if (new_node==NULL) {
		kprintf("Err -> Out of Memory\n");
		return NULL;
	}
	Token *break_k = eat();

	return new_node;
}

static ASTNode **parse_statement_list(size_t *out_s, LTokenType *stop_tokens, size_t n_stop_tokens) {
	ASTNode **stmt_list = NULL;
	size_t size = 0;
	size_t cap = 4;
	stmt_list = (ASTNode**)kmalloc(sizeof(ASTNode*)*cap);
	if (stmt_list == NULL) {
		kprintf("Err -> Out of Memory\n");
		*out_s = 0;
		return NULL;
	}
	while (peek()->type != TOKEN_EOF) {
		bool is_stop=false;
		for (size_t i=0; i<n_stop_tokens;i++) {
			if (peek()->type == stop_tokens[i]) {
				is_stop=true;
				break;
			}
		}
		if (is_stop) break;

		if (peek()->type == TOKEN_NEW_LINE) {
			eat();
			continue;
		}
		ASTNode *stmt = parse_statement();
		if (stmt==NULL) {
			for (size_t i=0;i<size;i++) {
				parser_free_ast(stmt_list[i]);
			}
			kfree(stmt_list);
			*out_s = 0;
			return NULL;
		}
		while (peek()->type == TOKEN_NEW_LINE) eat(); // token_new_line
		
		if (size >= cap) {
			cap *= 2;
			ASTNode **new_stmt_list = (ASTNode**)kmalloc(sizeof(ASTNode*)*cap);
			if (new_stmt_list==NULL) {
				kprintf("Err -> Out of Memory\n");
				parser_free_ast(stmt);
				for (size_t i=0;i<size;i++) {
					parser_free_ast(stmt_list[i]);
				}
				kfree(stmt_list);
				*out_s=0;
				return NULL;
			}
			memcpy((void*)new_stmt_list, (const void*)stmt_list, size*sizeof(ASTNode*));
			kfree(stmt_list);
			stmt_list = new_stmt_list;
		}
		stmt_list[size++]=stmt;
	}	
	*out_s = size;	
	return stmt_list;
}

static ASTNode *parse_if() {
	ASTNode *new_node = newASTNode(NODE_IF);
	if (new_node == NULL) {
		kprintf("Err -> Out of Memory\n");
		return NULL;
	}

	Token *if_k = eat();
	
	new_node->data.if_stmt.condition = parse_expression();
	if (!new_node->data.if_stmt.condition) {
		kfree(new_node);
		return NULL;
	}
	while (peek()->type == TOKEN_NEW_LINE) {
		eat();
	}
	if (!match(TOKEN_THEN)) {
		kprintf("Err -> Expected 'then' after condition\n");
		parser_free_ast(new_node);
		return NULL;
	}
	while (peek()->type == TOKEN_NEW_LINE) {
		eat();
	}
	LTokenType then_stop_toks[] = {TOKEN_FI, TOKEN_ELSE};

	new_node->data.if_stmt.thenb = parse_statement_list(&(new_node->data.if_stmt.s_then), then_stop_toks, 2);
	if (new_node->data.if_stmt.thenb == NULL || new_node->data.if_stmt.s_then==0) {
		parser_free_ast(new_node);
		return NULL;
	}
	if (peek()->type == TOKEN_ELSE) {
		/*
		 * SOCORROOOOOOOO
		 *
		 */
		eat();
		while (peek()->type==TOKEN_NEW_LINE) {
			eat();
		}
		LTokenType else_stop_toks[] = {TOKEN_FI};
		new_node->data.if_stmt.elseb = parse_statement_list(
				&(new_node->data.if_stmt.s_else),
				else_stop_toks,
				1
				);
		if (new_node->data.if_stmt.elseb == NULL || new_node->data.if_stmt.s_else == 0) {
			parser_free_ast(new_node);
			return NULL;
		}
	} else {
		new_node->data.if_stmt.elseb = NULL;
		new_node->data.if_stmt.s_else = 0;
	}
	while (peek()->type == TOKEN_NEW_LINE) eat();

	if (!match(TOKEN_FI)) {
		kprintf("Err -> Expected 'fi'\n");
		parser_free_ast(new_node);
		return NULL;
	}
	return new_node;
}

static ASTNode *parse_while() {
	ASTNode *new_node = newASTNode(NODE_WHILE_LOOP);
	if (new_node==NULL) {
		kprintf("Err -> Out of Memory\n");
		return NULL;
	}
	Token *do_k = eat();
	
	while (peek()->type == TOKEN_NEW_LINE) eat();

	if (!match(TOKEN_LBRACE)) {
		kprintf("Err -> Expected '{' after 'do'\n");
		kfree(new_node);
		return NULL;
	}

	while (peek()->type==TOKEN_NEW_LINE) eat();

	LTokenType body_stop[] = {TOKEN_RBRACE};
	new_node->data.while_loop.body = parse_statement_list(
			&(new_node->data.while_loop.s_body),
			body_stop,
			1
			);
	if (new_node->data.while_loop.body==NULL || new_node->data.while_loop.s_body == 0) {
		kprintf("Err -> Expected Statements before '}'\n");
		parser_free_ast(new_node);
		return NULL;
	}
	while (peek()->type==TOKEN_NEW_LINE) eat();

	if (!match(TOKEN_RBRACE)) {
		kprintf("Err -> Expected '}' after statements\n");
		parser_free_ast(new_node);
		return NULL;
	}
	Token *while_k = eat();

	new_node->data.while_loop.condition = parse_expression();
	if (new_node->data.while_loop.condition==NULL) {
		parser_free_ast(new_node);
		return NULL;
	}
	return new_node;
}

static ASTNode *parse_for() {
	ASTNode *new_node=newASTNode(NODE_FOR_LOOP);
	if (new_node==NULL) {
		kprintf("Err -> Out of Memory\n");
		return NULL;
	}

	Token *for_tok=eat();

	if (!match(TOKEN_LPAREN)) {
		kprintf("Err -> Expected '(' after 'for'\n");
		kfree(new_node);
		return NULL;
	}
	if (peek()->type != TOKEN_SEMICOLON) {
		new_node->data.for_loop.init = parse_statement();
		if (new_node->data.for_loop.init == NULL) {
			kfree(new_node);
			return NULL;
		}
	} else {
		new_node->data.for_loop.init = NULL;
	}
	
	if (!match(TOKEN_SEMICOLON)) {
		kprintf("Err -> Expected ';' after expression\n");
		parser_free_ast(new_node);
		return NULL;
	}

	if (peek()->type != TOKEN_SEMICOLON) {
		new_node->data.for_loop.condition = parse_expression();
		if (new_node->data.for_loop.condition==NULL) {
			parser_free_ast(new_node);
			return NULL;
		}
	} else {
		new_node->data.for_loop.condition=NULL;
	}

	if (!match(TOKEN_SEMICOLON)) {
		kprintf("Err -> Expected ';' after expression\n");
		parser_free_ast(new_node);
		return NULL;
	}

	if (peek()->type != TOKEN_RPAREN) {
		new_node->data.for_loop.inc = parse_statement();
		if (new_node->data.for_loop.inc == NULL) {
			parser_free_ast(new_node);
			return NULL;
		}
	} else {
		new_node->data.for_loop.inc = NULL;
	}

	if (!match(TOKEN_RPAREN)) {
		kprintf("Err -> Expected ')' after expression\n");
		parser_free_ast(new_node);
		return NULL;
	}

	while (peek()->type == TOKEN_NEW_LINE) eat();

	if (!match(TOKEN_DO)) {
		kprintf("Err -> Expected 'do' at the begining of a for loop\n");
		parser_free_ast(new_node);
		return NULL;
	}


	while (peek()->type == TOKEN_NEW_LINE) eat();

	LTokenType body_stop[] = {TOKEN_DONE};

	new_node->data.for_loop.body = parse_statement_list(
			&(new_node->data.for_loop.s_body),
			body_stop,
			1
			);
	if (new_node->data.for_loop.body == NULL || new_node->data.for_loop.s_body == 0) {
		parser_free_ast(new_node);
		return NULL;
	}

	while (peek()->type == TOKEN_NEW_LINE) eat();

	if (!match(TOKEN_DONE)) {
		kprintf("Err -> Expected 'done' to close a for loop\n");
		parser_free_ast(new_node);
		return NULL;
	}

	return new_node;
}


