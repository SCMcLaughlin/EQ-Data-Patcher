
#include "pfs.h"

static SimpleBuffer* pfs_decompress(Pfs* pfs, uint32_t i);

typedef struct PfsHeader {
    uint32_t    offset;
    uint32_t    signature;
    uint32_t    unknown;
} PfsHeader;

typedef struct PfsBlock {
    uint32_t    deflatedLen;
    uint32_t    inflatedLen;
} PfsBlock;

typedef struct PfsFileEntry {
    uint32_t    crc;
    uint32_t    offset;
    uint32_t    inflatedLen;
} PfsFileEntry;

typedef struct PfsEntry {
    uint32_t        crc;
    uint32_t        offset;
    uint32_t        inflatedLen;
    uint32_t        deflatedLen;
    SimpleString*   name;
    Array           replacement;
} PfsEntry;

Pfs* pfs_create(void)
{
    Pfs* pfs = alloc_type(Pfs);
    
    if (!pfs) return NULL;
    
    pfs_init(pfs);
    return pfs;
}

void pfs_destroy(Pfs* pfs)
{
    if (!pfs) return;
    
    pfs_deinit(pfs);
    free(pfs);
}

void pfs_init(Pfs* pfs)
{
    memset(pfs, 0, sizeof(Pfs));
    
    array_init(&pfs->entries, PfsEntry);
    tbl_init(&pfs->byName, uint32_t);
}

static void pfs_destroy_entry(void* ptr)
{
    PfsEntry* ent = (PfsEntry*)ptr;
    array_deinit(&ent->replacement, NULL);
}

void pfs_deinit(Pfs* pfs)
{
    array_deinit(&pfs->entries, pfs_destroy_entry);
    tbl_deinit(&pfs->byName, NULL);
    
    if (pfs->raw)
    {
        buf_destroy(pfs->raw);
        pfs->raw = NULL;
    }
    
    if (pfs->path)
    {
        sstr_destroy(pfs->path);
        pfs->path = NULL;
    }
}

#ifdef EDP_WINDOWS
static int64_t pfs_mod_time(const char* path)
{
    HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    FILETIME fTime;
    ULARGE_INTEGER uInt;

    if (hFile == INVALID_HANDLE_VALUE)
        return -1;

    if (!GetFileTime(hFile, NULL, NULL, &fTime))
    {
        CloseHandle(hFile);
        return -1;
    }

    uInt.LowPart    = fTime.dwLowDateTime;
    uInt.HighPart   = fTime.dwHighDateTime;

    return (int64_t)uInt.QuadPart;
}
#else
static int64_t pfs_mod_time(const char* path)
{
    struct stat fstat;
    
    if (stat(path, &fstat))
        return -1;
    
    return (int64_t)fstat.st_mtime;
}
#endif

static int pfs_cmp(const void* va, const void* vb)
{
    PfsEntry* a = (PfsEntry*)va;
    PfsEntry* b = (PfsEntry*)vb;
    
    return a->offset < b->offset;
}

int pfs_open(Pfs* pfs, const char* path)
{
    SimpleString* file = sstr_from_file(path);
    uint32_t len;
    byte* data;
    uint32_t p, n, i;
    PfsHeader* h;
    SimpleBuffer* nameData;
    
    if (!file) return ERR_OutOfMemory;
    
    len     = sstr_length(file);
    data    = (byte*)sstr_data(file);
    
    pfs->modTime    = pfs_mod_time(path);
    pfs->raw        = file;
    pfs->path       = sstr_create(path, strlen(path));
    
    if (!pfs->path) return ERR_OutOfMemory;
    
    p = sizeof(PfsHeader);
    
    if (p > len) goto bad_size;
    
    h = (PfsHeader*)data;
    
    if (memcmp(&h->signature, "PFS ", sizeof(uint32_t)) != 0)
        return ERR_Invalid;
    
    p = h->offset;
    
    if (p > len) goto bad_size;
    
    n = *(uint32_t*)(data + p);
    p += sizeof(uint32_t);
    
    for (i = 0; i < n; i++)
    {
        PfsFileEntry* src = (PfsFileEntry*)(data + p);
        PfsEntry ent;
        uint32_t memPos;
        uint32_t ilen, totalLen;
        uint32_t offset;
        
        memPos = p + sizeof(PfsFileEntry);
        
        if (memPos > len) goto bad_size;
        
        offset = src->offset;
        
        ent.crc         = src->crc;
        ent.offset      = offset;
        ent.inflatedLen = src->inflatedLen;
        ent.name        = NULL;
        array_init(&ent.replacement, byte);
        
        p           = offset;
        ilen        = 0;
        totalLen    = src->inflatedLen;
        
        while (ilen < totalLen)
        {
            PfsBlock* block = (PfsBlock*)(data + p);
            
            p += sizeof(PfsBlock);
            
            if (p > len) goto bad_size;
            
            p += block->deflatedLen;
            
            if (p > len) goto bad_size;
            
            ilen += block->inflatedLen;
        }
        
        ent.deflatedLen = p - offset;
        
        p = memPos;
        
        if (!array_push_back(&pfs->entries, &ent))
            return ERR_OutOfMemory;
    }
    
    if (array_sort(&pfs->entries, pfs_cmp))
        return ERR_OutOfMemory;
    
    n = array_count(&pfs->entries);
    
    if (n == 0)
        return ERR_Invalid;
    
    n--;
    nameData = pfs_decompress(pfs, n);
    
    if (!nameData) return ERR_OutOfMemory;
    
    array_pop_back(&pfs->entries);
    
    len     = buf_length(nameData);
    data    = (byte*)buf_data(nameData);
    
    if (len < sizeof(uint32_t)) goto bad_name;
    
    n = *(uint32_t*)data;
    p = sizeof(uint32_t);
    
    for (i = 0; i < n; i++)
    {
        PfsEntry* ent;
        uint32_t namelen;
        const char* name;
        int rc;
        
        if ((p + sizeof(uint32_t)) > len) goto bad_name;
        
        namelen = *(uint32_t*)(data + p);
        p += sizeof(uint32_t);
        
        name = (const char*)(data + p);
        p += namelen;
        
        if (p > len) goto bad_name;
        
        ent = array_get(&pfs->entries, i, PfsEntry);
        
        if (!ent)
        {
            /* This should not happen */
            rc = ERR_OutOfBounds;
            goto name_dest;
        }
        
        ent->name = sstr_create(name, namelen - 1);
        
        if (!ent->name)
        {
            rc = ERR_OutOfMemory;
            goto name_dest;
        }
        
        rc = tbl_set_str(&pfs->byName, name, namelen - 1, &i);
        
        if (rc)
        {
        name_dest:
            buf_destroy(nameData);
            return rc;
        }
    }
    
    buf_destroy(nameData);
    return ERR_None;
    
bad_name:
    buf_destroy(nameData);
bad_size:
    return ERR_OutOfBounds;
}

static byte* pfs_data(Pfs* pfs)
{
    return (byte*)sstr_data(pfs->raw);
}

static SimpleBuffer* pfs_decompress(Pfs* pfs, uint32_t i)
{
    PfsEntry* ent = array_get(&pfs->entries, i, PfsEntry);
    byte* src;
    byte* dst;
    uint32_t ilen;
    uint32_t read;
    uint32_t pos;
    SimpleBuffer* buf;
    
    if (!ent) return NULL;
    
    src     = pfs_data(pfs) + ent->offset;
    ilen    = ent->inflatedLen;
    read    = 0;
    pos     = 0;
    buf     = buf_create(NULL, ilen);
    
    if (!buf) return NULL;
    
    dst = (byte*)buf_data(buf);
    
    while (read < ilen)
    {
        PfsBlock* block = (PfsBlock*)(src + pos);
        unsigned long len;
        int rc;
        
        pos += sizeof(PfsBlock);
        
        len = ilen - read;
        rc  = uncompress(dst + read, &len, src + pos, block->deflatedLen);
        
        if (rc != Z_OK) goto fail;
        
        read += block->inflatedLen;
        pos += block->deflatedLen;
    }
    
    return buf;
    
fail:
    buf_destroy(buf);
    return NULL;
}

static int pfs_index_by_name(Pfs* pfs, const char* name, uint32_t len)
{
    uint32_t* index;
    
    if (len == 0)
        len = strlen(name);
    
    index = tbl_get_str(&pfs->byName, name, len, uint32_t);
    
    return (index) ? (int)*index : -1;
}

SimpleBuffer* pfs_get(Pfs* pfs, const char* name, uint32_t len)
{
    int index = pfs_index_by_name(pfs, name, len);
    
    if (index < 0) return NULL;
    
    return pfs_decompress(pfs, (uint32_t)index);
}

static PfsEntry* pfs_get_or_append_entry(Pfs* pfs, const char* name, uint32_t len)
{
    int index = pfs_index_by_name(pfs, name, len);
    PfsEntry* ent;
    PfsEntry dst;
    
    if (index >= 0)
        return array_get(&pfs->entries, (uint32_t)index, PfsEntry);
    
    /* Need to add a new entry */
    memset(&dst, 0, sizeof(PfsEntry));
    
    if (len == 0)
        len = strlen(name);
    
    dst.crc     = crc_calc(name, len);
    dst.name    = sstr_create(name, len);
    array_init(&dst.replacement, byte);
    
    if (!dst.name) return NULL;
    
    index   = array_count(&pfs->entries);
    ent     = array_push_back_type(&pfs->entries, &dst, PfsEntry);
    
    if (!ent) return NULL;
    
    /* Add the new entry to the hash table */
    if (tbl_set_str(&pfs->byName, name, len, &index))
    {
        array_pop_back(&pfs->entries);
        return NULL;
    }
    
    return ent;
}

#define PFS_COMPRESS_INPUT_SIZE 8192
#define PFS_COMPRESS_BUF_SIZE (PFS_COMPRESS_INPUT_SIZE + 128) /* Overflow space for things that can't be compressed any further... */

static int pfs_compress(PfsEntry* ent, const void* data, uint32_t len)
{
    const byte* ptr = (const byte*)data;
    byte tmp[PFS_COMPRESS_BUF_SIZE];
    
    ent->inflatedLen = len;
    
    while (len > 0)
    {
        uint32_t r = (len < PFS_COMPRESS_INPUT_SIZE) ? len : PFS_COMPRESS_INPUT_SIZE;
        unsigned long dstlen;
        PfsBlock block;
        int rc;
        
        block.inflatedLen   = r;
        dstlen              = sizeof(tmp);
        
        rc = compress2(tmp, &dstlen, ptr, r, Z_BEST_COMPRESSION);
        
        if (rc != Z_OK) return ERR_Compression;
        
        block.deflatedLen = dstlen;
        
        if (array_append(&ent->replacement, &block, sizeof(block)) || array_append(&ent->replacement, tmp, dstlen))
            return ERR_OutOfMemory;
        
        len -= r;
        ptr += r;
    }
    
    ent->deflatedLen = array_count(&ent->replacement);
    
    return ERR_None;
}

int pfs_put(Pfs* pfs, const char* name, uint32_t namelen, const void* data, uint32_t datalen)
{
    PfsEntry* ent = pfs_get_or_append_entry(pfs, name, namelen);
    
    if (!ent) return ERR_OutOfMemory;
    
    array_clear(&ent->replacement);
    return pfs_compress(ent, data, datalen);
}

const byte* pfs_get_compressed(Pfs* pfs, const char* name, uint32_t* len, uint32_t* inflatedLen)
{
    int index = pfs_index_by_name(pfs, name, *len);
    PfsEntry* ent;
    
    if (index < 0) return NULL;
    
    ent = array_get(&pfs->entries, (uint32_t)index, PfsEntry);
    
    if (!ent) return NULL;
    
    *len            = ent->deflatedLen;
    *inflatedLen    = ent->inflatedLen;
    return pfs_data(pfs) + ent->offset;
}

int pfs_put_compressed(Pfs* pfs, const char* name, uint32_t namelen, const void* data, uint32_t datalen, uint32_t inflatedLen)
{
    PfsEntry* ent = pfs_get_or_append_entry(pfs, name, namelen);
    
    if (!ent) return ERR_OutOfMemory;
    
    ent->deflatedLen = datalen;
    ent->inflatedLen = inflatedLen;
    
    array_clear(&ent->replacement);
    return array_append(&ent->replacement, data, datalen);
}

static int pfs_check_modified(Pfs* pfs)
{
    return pfs_mod_time(sstr_data(pfs->path)) > pfs->modTime;
}

static int pfs_crc_cmp(const void* va, const void* vb)
{
    PfsFileEntry* a = (PfsFileEntry*)va;
    PfsFileEntry* b = (PfsFileEntry*)vb;
    
    return a->crc < b->crc;
}

int pfs_save(Pfs* pfs)
{
    FILE* fp;
    PfsHeader header;
    PfsFileEntry fent;
    Array fileEntries;
    Array dataBuf;
    Array nameBuf;
    PfsEntry nameBufCompressed;
    uint32_t p, n, i, c;
    byte* pfsData   = pfs_data(pfs);
    int rc          = ERR_None;
    
    /* Early check: if the underlying file has been modified by an external program, abort */
    if (pfs_check_modified(pfs)) return ERR_Again;
    
    memcpy(&header.signature, "PFS ", sizeof(header.signature));
    header.unknown = 131072; /* Always this */
    
    p = sizeof(PfsHeader);
    c = array_count(&pfs->entries);
    
    array_init(&fileEntries, PfsFileEntry);
    array_init(&dataBuf, byte);
    array_init(&nameBuf, byte);
    array_init(&nameBufCompressed.replacement, byte);
    
    if (array_append(&nameBuf, &c, sizeof(uint32_t)))
        goto mem_err;
    
    for (i = 0; i < c; i++)
    {
        PfsEntry* ent = array_get(&pfs->entries, i, PfsEntry);
        
        if (!ent)
        {
            rc = ERR_OutOfBounds;
            goto abort;
        }
        
        if (!ent->name)
        {
            rc = ERR_Invalid;
            goto abort;
        }
        
        n = sstr_length(ent->name) + 1;
        if (array_append(&nameBuf, &n, sizeof(uint32_t)) || array_append(&nameBuf, sstr_data(ent->name), n))
            goto mem_err;
        
        fent.crc            = ent->crc;
        fent.offset         = p;
        fent.inflatedLen    = ent->inflatedLen;
        
        if (!array_push_back(&fileEntries, &fent))
            goto mem_err;
        
        if (array_empty(&ent->replacement))
        {
            if (array_append(&dataBuf, pfsData + ent->offset, ent->deflatedLen))
                goto mem_err;
        }
        else
        {
            if (array_append(&dataBuf, array_raw(&ent->replacement), ent->deflatedLen))
                goto mem_err;
        }
        
        p += ent->deflatedLen;
    }
    
    /* Names entry */
    fent.crc            = 0x61580ac9; /* Always this */
    fent.offset         = p;
    fent.inflatedLen    = array_count(&nameBuf);
    
    if (!array_push_back(&fileEntries, &fent) || array_sort(&fileEntries, pfs_crc_cmp))
        goto mem_err;
    
    rc = pfs_compress(&nameBufCompressed, array_raw(&nameBuf), array_count(&nameBuf));
    
    if (rc) goto abort;
    
    p += array_count(&nameBufCompressed.replacement);
    
    header.offset = p;
    
    /* Before we open the file for writing, check if it has been modified by an external program while we were working, and abort if so */
    if (pfs_check_modified(pfs)) return ERR_Again;
    
    fp = fopen(sstr_data(pfs->path), "wb+");
    
    if (!fp) goto mem_err;
    
    /* Header */
    if (fwrite(&header, sizeof(byte), sizeof(header), fp) != sizeof(header))
        goto file_err;
    
    /* Compressed entries */
    n = array_count(&dataBuf);
    if (fwrite(array_raw(&dataBuf), sizeof(byte), n, fp) != n)
        goto file_err;
    
    /* Compressed names entry */
    n = array_count(&nameBufCompressed.replacement);
    if (fwrite(array_raw(&nameBufCompressed.replacement), sizeof(byte), n, fp) != n)
        goto file_err;
    
    /* Offset and CRC list in order of CRC */
    n = array_count(&fileEntries);
    if (fwrite(&n, sizeof(byte), sizeof(uint32_t), fp) != sizeof(uint32_t))
        goto file_err;
    
    for (i = 0; i < n; i++)
    {
        PfsFileEntry* ent = array_get(&fileEntries, i, PfsFileEntry);
        
        if (!ent)
        {
            rc = ERR_OutOfBounds;
            break;
        }
        
        if (fwrite(ent, sizeof(byte), sizeof(PfsFileEntry), fp) != sizeof(PfsFileEntry))
            goto file_err;
    }
    
close_file:
    fclose(fp);
    
abort:
    array_deinit(&fileEntries, NULL);
    array_deinit(&dataBuf, NULL);
    array_deinit(&nameBuf, NULL);
    array_deinit(&nameBufCompressed.replacement, NULL);
    
    return rc;
    
mem_err:
    rc = ERR_OutOfMemory;
    goto abort;
    
file_err:
    rc = ERR_FileOperation;
    goto close_file;
}
