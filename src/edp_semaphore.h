
#ifndef EDP_SEMAPHORE_H
#define EDP_SEMAPHORE_H

#include "define.h"
#include "structs.h"

/*
    "Unnamed" Semaphore, no filesystem representation.
*/

int semaphore_init(Semaphore* ptr);
int semaphore_deinit(Semaphore* ptr);

int semaphore_wait(Semaphore* ptr);
int semaphore_try_wait(Semaphore* ptr);
int semaphore_trigger(Semaphore* ptr);

#endif/*EDP_SEMAPHORE_H*/
