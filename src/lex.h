
#ifndef LEX_H
#define LEX_H

#include "define.h"
#include "structs.h"
#include "container.h"
#include "enum_lex.h"

void lex_init(Lexer* lex, SimpleString* str);
void lex_deinit(Lexer* lex);

int lex_adv(Lexer* lex);
int lex_peek(Lexer* lex);

const char* lex_str(Lexer* lex);
uint32_t lex_len(Lexer* lex);

#endif/*LEX_H*/
