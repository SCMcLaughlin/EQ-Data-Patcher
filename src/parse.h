
#ifndef PARSE_H
#define PARSE_H

#include "define.h"
#include "structs.h"
#include "container.h"
#include "lex.h"
#include "enum_parse.h"

void parse_init(Parser* p);
void parse_deinit(Parser* p);

int parse_file(Parser* p, SimpleString* str);

#endif/*PARSE_H*/
