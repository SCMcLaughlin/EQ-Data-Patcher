
#ifndef DB_H
#define DB_H

#include "define.h"
#include "structs.h"
#include "sqlite3.h"
#include "edp_string.h"

#define DB_PATH     "data/db/edp.db"
#define SCHEMA_PATH "data/db/edp_schema.sql"

int db_init(sqlite3** db);
void db_deinit(sqlite3* db);

int db_exec(sqlite3* db, const char* sql);

#endif/*DB_H*/
