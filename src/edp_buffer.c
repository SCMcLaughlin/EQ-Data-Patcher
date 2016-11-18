
#include "edp_buffer.h"

SimpleBuffer* buf_create(const void* data, uint32_t len)
{
    byte* ptr       = alloc_bytes(len + sizeof(uint32_t));
    uint32_t* plen  = (uint32_t*)ptr;
    
    if (!ptr) return NULL;
    
    *plen = len;
    
    if (data)
        memcpy(ptr + sizeof(uint32_t), data, len);
    
    return (SimpleBuffer*)ptr;
}

void buf_destroy(SimpleBuffer* buf)
{
    free(buf);
}

uint32_t buf_length(SimpleBuffer* buf)
{
    return *(uint32_t*)buf;
}

const byte* buf_data(SimpleBuffer* buf)
{
    return ((byte*)buf) + sizeof(uint32_t);
}
