
#include "err_code.h"

const char* err_str(int rc)
{
    switch (rc)
    {
    case ERR_False: return "ERR_False";
    case ERR_None: return "ERR_None";
    case ERR_Generic: return "ERR_Generic";
    case ERR_CouldNotInit: return "ERR_CouldNotInit";
    case ERR_CouldNotOpen: return "ERR_CouldNotOpen";
    case ERR_Invalid: return "ERR_Invalid";
    case ERR_OutOfMemory: return "ERR_OutOfMemory";
    case ERR_OutOfSpace: return "ERR_OutOfSpace";
    case ERR_Again: return "ERR_Again";
    case ERR_NotInitialized: return "ERR_NotInitialized";
    case ERR_CouldNotCreate: return "ERR_CouldNotCreate";
    case ERR_Semaphore: return "ERR_Semaphore";
    case ERR_FileOperation: return "ERR_FileOperation";
    default: return "ERR_Unknown";
    }
}
