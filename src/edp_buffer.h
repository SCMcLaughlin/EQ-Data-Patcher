
#ifndef EDP_BUFFER_H
#define EDP_BUFFER_H

#include "define.h"
#include "structs.h"
#include "edp_alloc.h"

SimpleBuffer* buf_create(const void* data, uint32_t len);
void buf_destroy(SimpleBuffer* buf);

uint32_t buf_length(SimpleBuffer* buf);
const byte* buf_data(SimpleBuffer* buf);
#define buf_data_type(buf, type) (type*)buf_data((buf))

#endif/*EDP_BUFFER_H*/
