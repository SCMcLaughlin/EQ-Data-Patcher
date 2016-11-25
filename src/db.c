
#include "db.h"

int db_init(sqlite3** db)
{
    int rc = sqlite3_open_v2(
        DB_PATH,
        db,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX,
        NULL
    );
    
    if (rc == SQLITE_OK)
    {
        db_exec(*db, "ANALYZE;");
        return ERR_None;
    }
    
    if (rc == SQLITE_CANTOPEN)
    {
        SimpleString* sql;
        
        rc = sqlite3_open_v2(
            DB_PATH,
            db,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_CREATE,
            NULL
        );
    
        if (rc != SQLITE_OK) return ERR_CouldNotCreate;
        
        sql = sstr_from_file(SCHEMA_PATH);
        
        if (!sql) return ERR_CouldNotOpen;
        
        rc = db_exec(*db, sstr_data(sql));
        sstr_destroy(sql);
        return rc;
    }
    
    return ERR_CouldNotInit;
}

void db_deinit(sqlite3* db)
{
    sqlite3_close_v2(db);
}

int db_exec(sqlite3* db, const char* sql)
{
    int rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
    return (rc == SQLITE_OK) ? ERR_None : ERR_Invalid;
}
