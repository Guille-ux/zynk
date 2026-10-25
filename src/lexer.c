#include "../include/lexer.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

bool debug=false;
static const char *source;
static size_t current_pos;


static char *m_strndup(const char *str, size_t n) {
	char *ret=(char*)kmalloc(n+1);
	memcpy((void*)ret, (const void*)str, n);
	ret[n]='\0';
	return ret;
}

static char advance() {
	if (source[current_pos]=='\0') return '\0';
	return source[current_pos++];
}

static char next() {
	return source[current_pos+1];
}

static char peek() {
	return source[current_pos];
}

static Token newToken(LTokenType type, const char *start, size_t len) {
	Token token;
	token.type = type;
	if (len==0) {
		return token;
	} 
	

	token.value=m_strndup(start, len);
	if (debug==true) {
		kprintf("New Token -> %s \n", (const char *)token.value);
	}
	return token;
}

void lexer_init(const char *source_code) {
	source=source_code;
	current_pos=0;
}

void lexer_free_token(Token *token) {
    if (token && token->value) {
        kfree(token->value);
        token->value = NULL;
		token->type = TOKEN_UNKNOWN;
    }
}

/*
 * Es un lexer muy simple, pretendo mejorarlo, esto es provisional
 * lo justo y necesario para la shell.
 *
 * Estoy deseando sustituir minim por esto!!!!
 *
 * Guillermo Leira Temes - 2025
 */

static bool match(char c) {
	if (peek()==c) {
		advance();
		return true;
	} 
	return false;
}

static bool pattern(const char *str) {
	size_t last = current_pos;
	const char *s = str;
	bool tmp = false;
	
	while (*s != '\0') {
		if (peek() == *s) {
			advance();
			s++;
		} else {
			break;
		}
	}
	
	if (*s != '\0') {
		// oh, q mal, no es
		current_pos = last;
		return false;
	}
	// JAJAJAJAJ, CREO Q ESTO LO ARREGLA
	// si es una letra o num entonces es un identificador más largo creo
	if (k_isalnum(peek()) || peek() == '_') {
		current_pos = last;
		return false;
	}

	return true;
}

Token lexer_next_token() {
	char c;
	while (peek() != '\0') {
		c = peek();	
		if (pattern("<=")) return newToken(TOKEN_LEQ, "<=", 2);
		if (pattern(">=")) return newToken(TOKEN_GEQ, ">=", 2);
		if (pattern("!=")) return newToken(TOKEN_BEQ, "!=", 2);
		switch (c) {
			case '\n': advance(); return newToken(TOKEN_NEW_LINE, "\n", 1);
			case '*': advance(); return newToken(TOKEN_MUL, "*", 1);
			case '/': advance(); return newToken(TOKEN_DIV, "/", 1);
			case '-': advance(); return newToken(TOKEN_MINUS, "-", 1);
			case '(': advance(); return newToken(TOKEN_LPAREN, "(", 1);
			case ')': advance(); return newToken(TOKEN_RPAREN, ")", 1);
			case '[': advance(); return newToken(TOKEN_LBRACKET, "[", 1);
			case ']': advance(); return newToken(TOKEN_RBRACKET, "]", 1);
			case '{': advance(); return newToken(TOKEN_LBRACE, "{", 1);
			case '}': advance(); return newToken(TOKEN_RBRACE, "}", 1);
			case '$': advance(); return newToken(TOKEN_DOLLAR, "$", 1);
			case '+': advance(); return newToken(TOKEN_PLUS, "+", 1);
			case ';': advance(); return newToken(TOKEN_SEMICOLON, ";", 1);
			case ',': advance(); return newToken(TOKEN_COMMA, ",", 1);
			case ':': advance(); return newToken(TOKEN_COLON, ":", 1);
			case '.': advance(); return newToken(TOKEN_DOT, ".", 1);
			case '!': 
				advance();
				if (peek()=='=') {
					advance();
					return newToken(TOKEN_BEQ, "!=", 2);
				}
				return newToken(TOKEN_BANG, "!", 1);
			case '<': 
				advance();
				if (peek()=='=') {
					advance();
					return newToken(TOKEN_LEQ, "<=", 2);
				}
				return newToken(TOKEN_LESS, "<", 1);
			case '>':
				advance();
				if (peek()=='=') {
					advance();
					return newToken(TOKEN_GEQ, ">=", 2);
				}
				return newToken(TOKEN_GREATER, ">", 1);
			case '|': {
				advance();
				if (peek()=='|') {
					advance();
					return newToken(TOKEN_DPIPE, "||", 2);
				} else {
					return newToken(TOKEN_PIPE, "|", 1);
				}
				  }
			default: break;
		}
		if (k_isspace(c)) {
			while (k_isspace(peek())) advance();
			continue;
		}
		if (c=='#') {
			while (peek() != '\n' && peek() != '\0') advance();
			continue;
		}
		if (c=='\'') {
			advance();
			size_t start_pos=current_pos;
			while (peek() != '\'' && peek() != '\0') {
				advance();
			}
			if (peek() == '\0') return newToken(TOKEN_UNKNOWN, "Unterminated String!", 20);
			Token string_tok=newToken(TOKEN_STRING, source + start_pos, current_pos - start_pos);
			advance();
			return string_tok;
		}
		if (c=='"') {
			advance();
			size_t start_pos=current_pos;
			while (peek() != '"' && peek() != '\0') {	
				advance();
			}
			if (peek() == '\0') return newToken(TOKEN_UNKNOWN, "Unterminated String!", 20);
			char *buffer = kmalloc(current_pos-start_pos+1);
			buffer[current_pos-start_pos]='\0';
			current_pos = start_pos;
			int i=0;
			while (peek() != '"' && peek() != '\0') {
				if (peek()=='\\') {
					advance();
					if (peek()=='\\') {
						buffer[i] = '\\';
					} else if (peek()=='n') {
						buffer[i] = '\n';
					} else if (peek()=='r') {
						buffer[i] = '\r';
					}
				} else {
					buffer[i] = peek();
				}
				i++;
				advance();
			}
			if (peek() == '\0') return newToken(TOKEN_UNKNOWN, "Unterminated String!", 20);

			Token string_tok=newToken(TOKEN_STRING, buffer, i);
			advance();
			return string_tok;
		}
		if (c=='=') {
			advance();
			if (peek()=='=') {
				advance();
				return newToken(TOKEN_EQ, "==", 2);
			}
			return newToken(TOKEN_ASSIGN, "=", 1);
		}
		
		if (k_isdigit(c)) { // NOTA: añadir soporte para decimales, en el futuro	
			bool comma = false;
			size_t start_pos=current_pos;
			while (k_isdigit(peek())||((peek()=='.')&&(!comma))) {
				if (peek()=='.') comma=true;
				advance();
			}
			return newToken(TOKEN_NUMBER, source + start_pos, current_pos - start_pos);
		}

		/*
		 * Patterns, son tokens internos, por ejemplo para break
		 * xD
		 */
		
		if (pattern("del")) return newToken(TOKEN_DEL, "del", 3);
		if (pattern("for")) return newToken(TOKEN_FOR, "for", 3);
		if (pattern("break")) return newToken(TOKEN_BREAK, "break", 5);
		if (pattern("if")) return newToken(TOKEN_IF, "if", 2);

		if (pattern("fi")) return newToken(TOKEN_FI, "fi", 2);
		if (pattern("while")) return newToken(TOKEN_WHILE, "while", 5);
		if (pattern("else")) return newToken(TOKEN_ELSE, "else", 4);
		if (pattern("then")) return newToken(TOKEN_THEN, "then", 4);
		if (pattern("echo")) return newToken(TOKEN_ECHO, "echo", 4);
		if (pattern("export")) return newToken(TOKEN_EXPORT, "export", 6);

		if (pattern("return")) return newToken(TOKEN_RETURN, "return", 6);
		if (pattern("null")) return newToken(TOKEN_NULL, "null", 4);
		if (pattern("done")) return newToken(TOKEN_DONE, "done", 4);
		if (pattern("do")) return newToken(TOKEN_DO, "do", 2);
		

		if (pattern("or")) return newToken(TOKEN_OR, "or", 2);
		if (pattern("and")) return newToken(TOKEN_AND, "and", 3);
		if (pattern("let")) return newToken(TOKEN_LET, "let", 3);
		if (pattern("fn")) return newToken(TOKEN_FN, "fn", 2);
		if (pattern("class")) return newToken(TOKEN_CLASS, "class", 4);		
		/*
		 * Añadir más tokens de este tipo, algo me dice que seran importantes
		 * Podría poner aqui los comandos internos, no se
		 * Creo que no, que los maneje la shell
		 *
		 */

		if (k_isalpha(c) || c=='_') {
			size_t start_pos = current_pos;
			while (k_isalnum(peek()) || peek() == '_') advance();
			return newToken(TOKEN_IDENTIFIER, source + start_pos, current_pos - start_pos);
		}
		c = advance();
		return newToken(TOKEN_UNKNOWN, &c, 1);
	}
	return newToken(TOKEN_EOF, "", 0);
}

Token *lexe(Token *ptr) {
	if (ptr != NULL) return NULL;
	
	size_t capacity = 128; // Empezamos por un número cómodo de tokens
	size_t count = 0;
	
	Token *buffer = malloc(capacity * sizeof(Token));
	if (buffer == NULL) {
		kprintf("Err -> Out of Memory in Lexer\n");
		return NULL;
	}

	Token t;
	do {
		if (count >= capacity) {
			capacity *= 2;
			Token *new_buffer = realloc(buffer, capacity * sizeof(Token));
			if (new_buffer == NULL) {
				kprintf("Err -> Out of Memory (realloc) in Lexer\n");
				// Liberar lo anterior si hace falta
				return buffer;
			}
			buffer = new_buffer;
		}
		
		t = lexer_next_token();
		buffer[count] = t;
		
		if (debug) {
			kprintf("Token: %s\n", t.value ? t.value : "(null)");
			kprintf("Token Type: %d\n", (int)t.type);
		}
		
		count++;
	} while (t.type != TOKEN_EOF);

	return buffer;
}

Token *lexen(Token *buffer, size_t n) {
	Token *b=buffer;
	do {
		*b=lexer_next_token();
		if (debug) {
			kprintf("Token: %s\n", b->value);
			kprintf("Token Type: %d\n", (int)b->type);
		}
		b++;
	} while ((b-1)->type!=TOKEN_EOF && n-- > 0);
	return buffer;
}

void lexer_free(Token *buffer) {
	size_t i=0;
	while (buffer[i].type!=TOKEN_EOF) {
		lexer_free_token(&buffer[i++]);
	}
}

void free_tokens(Token *buffer, size_t n) {
	for (size_t i=0;i<n;i++) {
		lexer_free_token(&buffer[i]);
	}
}
