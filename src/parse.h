
#ifndef PARSE_H
#define PARSE_H

#include "define.h"
#include "structs.h"
#include "container.h"
#include "lex.h"
#include "enum_parse.h"

void parse_init(Parser* p);
void parse_deinit(Parser* p);

void parse_deinit_each_patch_entry(void* ptr);
int parse_file(Parser* p, const char* str, uint32_t len);

Array* parse_get_manifests(Parser* p);

#endif/*PARSE_H*/
