
#include "ringbuf.h"

RingBuf* ringbuf_create(void)
{
    RingBuf* rb = alloc_type(RingBuf);
    
    if (!rb) return NULL;
    
    ringbuf_init(rb);
    return rb;
}

void ringbuf_init(RingBuf* rb)
{
    uint32_t i;
    uint32_t n          = RINGBUF_MAX_PACKETS - 1;
    RingBlock* blocks   = rb->blocks;
    
    blocks[0].nextIndex = 1;
    aint16_set(&blocks[0].hasBeenRead, 0);
    aint16_set(&blocks[0].hasBeenWritten, 0);
    
    for (i = 1; i < n; i++)
    {
        blocks[i].nextIndex = i + 1;
        aint16_set(&blocks[i].hasBeenRead, 0);
        aint16_set(&blocks[i].hasBeenWritten, 0);
    }
    
    blocks[n].nextIndex = 0;
    aint16_set(&blocks[n].hasBeenRead, 0);
    aint16_set(&blocks[n].hasBeenWritten, 0);
    
    aint16_set(&rb->readStart, 0);
    aint16_set(&rb->readEnd, 0);
    aint16_set(&rb->writeStart, 0);
    aint16_set(&rb->writeEnd, 0);
}

static void ringbuf_push_impl(RingBuf* rb, RingBlock* block, const RingPacket* p)
{
    memcpy(&block->packet, p, sizeof(RingPacket));
    aint16_set(&block->hasBeenWritten, 1);
    
    /* Advance over completely written blocks, as far as possible */
    for (;;)
    {
        int16_t index   = aint16_get(&rb->writeStart);
        block           = &rb->blocks[index];
        
        /*
            If the hasBeenWritten flag has already been cleared, another thread must have already advanced for us
            Or, we've reached the end of what was available
        */
        if (!aint16_cmp_xchg_strong(&block->hasBeenWritten, 1, 0))
            break;
        
        /* Advance writeStart and loop */
        aint16_cmp_xchg_strong(&rb->writeStart, index, block->nextIndex);
    }
}

int ringbuf_push(RingBuf* rb, const RingPacket* p)
{
    for (;;)
    {
        int16_t index       = aint16_get(&rb->writeEnd);
        RingBlock* block    = &rb->blocks[index];
        
        if (block->nextIndex == aint16_get(&rb->readStart))
            return ERR_OutOfSpace;
        
        if (aint16_cmp_xchg_weak(&rb->writeEnd, index, block->nextIndex))
        {
            ringbuf_push_impl(rb, block, p);
            return ERR_None;
        }
    }
}

static void ringbuf_pop_impl(RingBuf* rb, RingBlock* block, RingPacket* p)
{
    memcpy(p, &block->packet, sizeof(RingPacket));
    aint16_set(&block->hasBeenRead, 1);
    
    /* Advance over completely read blocks, as far as possible */
    for (;;)
    {
        int16_t index   = aint16_get(&rb->readStart);
        block           = &rb->blocks[index];
        
        /*
            If the hasBeenRead flag has already been cleared, another thread must have already advanced for us
            Or, we've reached the end of what was available
        */
        if (!aint16_cmp_xchg_strong(&block->hasBeenRead, 1, 0))
            return;
        
        /* Advance readStart and loop */
        aint16_cmp_xchg_strong(&rb->readStart, index, block->nextIndex);
    }
}

int ringbuf_pop(RingBuf* rb, RingPacket* p)
{
    for (;;)
    {
        int16_t index       = aint16_get(&rb->readEnd);
        RingBlock* block    = &rb->blocks[index];
        
        if (index == aint16_get(&rb->writeStart))
            return ERR_Again;
        
        if (aint16_cmp_xchg_weak(&rb->readEnd, index, block->nextIndex))
        {
            ringbuf_pop_impl(rb, block, p);
            return ERR_None;
        }
    }
}

void ring_packet_init(RingPacket* p, int opcode, uint32_t len, void* data)
{
    p->opcode   = opcode;
    p->length   = len;
    p->data     = data;
}
