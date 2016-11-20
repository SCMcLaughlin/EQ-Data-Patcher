
#include "edp_semaphore.h"

int semaphore_init(Semaphore* ptr)
{
    ptr->handle = CreateSemaphore(NULL, 0, INT_MAX, NULL);
    return (!ptr->handle) ? ERR_CouldNotInit : ERR_None;
}

int semaphore_deinit(Semaphore* ptr)
{
    HANDLE h    = ptr->handle;
    ptr->handle = NULL;
    
    return (h && !CloseHandle(h)) ? ERR_Semaphore : ERR_None;
}

int semaphore_wait(Semaphore* ptr)
{
    return WaitForSingleObject(ptr->handle, INFINITE) ? ERR_Semaphore : ERR_None;
}

int semaphore_try_wait(Semaphore* ptr)
{
    return WaitForSingleObject(ptr->handle, 0) ? ERR_Semaphore : ERR_None;
}

int semaphore_trigger(Semaphore* ptr)
{
    return ReleaseSemaphore(ptr->handle, 1, NULL) ? ERR_None : ERR_Semaphore;
}
