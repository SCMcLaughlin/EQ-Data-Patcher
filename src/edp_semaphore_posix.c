
#include "edp_semaphore.h"

int semaphore_init(Semaphore* ptr)
{
    return sem_init(&ptr->semaphore, 0, 0) ? ERR_CouldNotInit : ERR_None;
}

int semaphore_deinit(Semaphore* ptr)
{
    return sem_destroy(&ptr->semaphore) ? ERR_Semaphore : ERR_None;
}

int semaphore_wait(Semaphore* ptr)
{
    return sem_wait(&ptr->semaphore) ? ERR_Semaphore : ERR_None;
}

int semaphore_try_wait(Semaphore* ptr)
{
    if (sem_trywait(&ptr->semaphore))
    {
        int err = errno;
        return (err == EAGAIN) ? ERR_False : ERR_Semaphore;
    }
    
    return ERR_True;
}

int semaphore_trigger(Semaphore* ptr)
{
    return sem_post(&ptr->semaphore) ? ERR_Semaphore : ERR_None;
}
