
#ifndef RINGBUF_H
#define RINGBUF_H

#include "define.h"
#include "structs.h"
#include "edp_alloc.h"
#include "edp_atomic.h"
#include "enum_ringbuf.h"

RingBuf* ringbuf_create(void);
#define ringbuf_destroy(rb) free((rb))

void ringbuf_init(RingBuf* rb);

int ringbuf_push(RingBuf* rb, const RingPacket* p);
int ringbuf_pop(RingBuf* rb, RingPacket* p);

void ring_packet_init(RingPacket* p, int opcode, uint32_t len, void* data);

#endif/*RINGBUF_H*/
