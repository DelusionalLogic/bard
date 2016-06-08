#include "parser.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "myerror.h"
#include "logger.h"

struct Parser {
	const char* str;
	char cur;
	size_t index;
	size_t len;
};

void parser_start(struct Parser* parser, const char* str) {
	parser->str = str;
	parser->index = 0;
	parser->len = strlen(str);
	parser->cur = parser->len == 0 ? '\0' : parser->str[parser->index];
}

bool parser_isDone(struct Parser* parser) {
	return parser->index > parser->len;
}

void parser_eat(struct Parser* parser) {
	if(parser_isDone(parser))
		return;
	parser->index++;

	parser->cur = parser_isDone(parser) ? '\0' : parser->str[parser->index];
}

int parseArrayData(struct Parser * p, struct Node* node);

//TODO: Handle Errors
int parser_compileStr(const char* str, Vector* nodes) {
	vector_init_new(nodes, sizeof(struct Node), 8);

	struct Parser parser;
	struct Parser* p = &parser;
	parser_start(p, str);

	while(!parser_isDone(p)) {
		struct Node n = {};
		if(p->cur == '$') {
			parser_eat(p);
			parseArrayData(p, &n);
			parser_eat(p);
		} else {
			Vector curLit;
			vector_init_new(&curLit, sizeof(char), 16);
			while(p->cur != '$' && !parser_isDone(p)) {
				if(p->cur == '\\') {
					parser_eat(p);
					if(p->cur == '\\') {
						vector_putListBack_new(&curLit, "\\", 1);
						parser_eat(p);
					}else if(p->cur == '$') {
						vector_putListBack_new(&curLit, "$", 1);
						parser_eat(p);
					}else{
						log_write(LEVEL_ERROR, "Unknown escape character %c", p->cur);
						return MYERR_USERINPUTERR;
					}
				} else {
					vector_putBack_new(&curLit, &p->cur);
					parser_eat(p);
				}
			}
			vector_putListBack_new(&curLit, "\0", 1);
			n.type = NT_STRING;
			n.str.lit = vector_detach(&curLit);
		}
		vector_putBack_new(nodes, &n);
	}
	return 0;
}

int parseArrayData(struct Parser * p, struct Node* node) {
	node->type = NT_ARRAY;
	{
		Vector ident;
		vector_init_new(&ident, sizeof(char), 12);

		while(p->cur != '[' && !parser_isDone(p)) {
			vector_putBack_new(&ident, &p->cur);
			parser_eat(p);
		}
		vector_putListBack_new(&ident, "\0", 1);

		node->arr.arr = vector_detach(&ident);
		if(node->arr.arr == NULL)
			return MYERR_BASE; //TODO: ERROCODE
	}

	parser_eat(p); //Eat the [

	{
		Vector key;
		vector_init_new(&key, sizeof(char), 12);

		while(p->cur != ']' && !parser_isDone(p)) {
			vector_putBack_new(&key, &p->cur);
			parser_eat(p);
		}
		vector_putListBack_new(&key, "\0", 1);

		node->arr.key = vector_detach(&key);
		if(node->arr.key == NULL)
			return MYERR_BASE; //TODO: ERROCODE
	}

	return 0;
}
