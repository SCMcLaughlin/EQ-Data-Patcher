
#ifndef STRUCTS_H
#define STRUCTS_H

#include "define.h"

typedef void(*ElemCallback)(void*);
typedef int(*CmpCallback)(const void*, const void*);

/* Low-level containers */

typedef struct SimpleBuffer SimpleBuffer;
typedef struct SimpleBuffer SimpleString;

typedef struct Array {
    uint32_t    count;
    uint32_t    capacity;
    uint32_t    elemSize;
    byte*       data;
} Array;

typedef struct String {
    uint32_t    length;
    uint32_t    capacity;
    char*       data;
} String;

typedef struct HashTblEnt {
    union {
        SimpleString*   keyStr; /* The hash table makes private copies of all string keys */
        int64_t         keyInt;
    };
    uint32_t    hash;
    uint32_t    next;
    byte        data[0];
} HashTblEnt;

typedef struct HashTbl {
    uint32_t    capacity;
    uint32_t    elemSize;
    uint32_t    freeIndex;
    uint32_t    entSize;    /* Ensures each HashTblEnt will be a multiple of 8 bytes */
    byte*       data;
} HashTbl;

/* High-level containers */

typedef struct Pfs {
    Array           entries;
    HashTbl         byName;
    int64_t         modTime;
    SimpleString*   raw;
    SimpleString*   path;
} Pfs;

#endif/*STRUCTS_H*/
