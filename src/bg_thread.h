
#ifndef BG_THREAD_H
#define BG_THREAD_H

#include "define.h"
#include "structs.h"
#include "ringbuf.h"
#include "edp_semaphore.h"

int bg_thread_start(void);
void bg_thread_stop(void);

int bg_thread_send(const RingPacket* rp);
int bg_thread_recv(RingPacket* rp);

#endif/*BG_THREAD_H*/
