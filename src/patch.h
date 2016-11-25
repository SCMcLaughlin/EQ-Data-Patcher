
#ifndef PATCH_H
#define PATCH_H

#include "define.h"
#include "structs.h"
#include "container.h"
#include "parse.h"
#include "db.h"
#include "ringbuf.h"
#include <curl/curl.h>

int patch_download_manifests(Array* patches, HashTbl* byName, sqlite3* db);

void patch_download_and_apply(Array* patches, HashTbl* byName, sqlite3* db, SimpleString* eqPath, RingBuf* output, SimpleString* patchName);

#endif/*PATCH_H*/
