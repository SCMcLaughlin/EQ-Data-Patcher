
#ifndef PFS_H
#define PFS_H

#include "define.h"
#include "structs.h"
#include "container.h"
#include "edp_alloc.h"
#include "crc.h"
#include <zlib.h>

Pfs* pfs_create(void);
void pfs_destroy(Pfs* pfs);

void pfs_init(Pfs* pfs);
void pfs_deinit(Pfs* pfs);

int pfs_open(Pfs* pfs, const char* path);
int pfs_save(Pfs* pfs);

SimpleBuffer* pfs_get(Pfs* pfs, const char* name, uint32_t len);
int pfs_put(Pfs* pfs, const char* name, uint32_t namelen, const void* data, uint32_t datalen);

const byte* pfs_get_compressed(Pfs* pfs, const char* name, uint32_t* len, uint32_t* inflatedLen);
int pfs_put_compressed(Pfs* pfs, const char* name, uint32_t namelen, const void* data, uint32_t datalen, uint32_t inflatedLen);

#endif/*PFS_H*/
