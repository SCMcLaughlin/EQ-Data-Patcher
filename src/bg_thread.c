
#include "bg_thread.h"

typedef struct BG {
    sqlite3*    db;
    Array       patches;
    HashTbl     patchesByName;
    RingBuf*    input;
    RingBuf*    output;
    Semaphore   semaphore;
#ifdef EDP_WINDOWS
    HANDLE      hThread;
#else
    pthread_t   pthread;
#endif
} BG;

static BG sBG;

#ifdef EDP_WINDOWS
static DWORD WINAPI
#else
static void*
#endif
bg_thread_proc(void* unused)
{
    int rc;
    
    (void)unused;
    
    rc = db_init(&sBG.db);
    printf("db_init: %i\n", rc);
    
    if (rc)
    {
        RingPacket rp;
        ring_packet_init_value(&rp, RingOp_CouldNotOpenDB, rc);
        ringbuf_push(sBG.output, &rp);
    }
    
    for (;;)
    {
        RingPacket rp;
        
        if (semaphore_wait(&sBG.semaphore))
            break;
        
        while (!ringbuf_pop(sBG.input, &rp))
        {
            switch (rp.opcode)
            {
            case RingOp_Terminate:
                goto terminate;
            
            default:
                break;
            }
        }
    }
    
terminate:
    
#ifdef EDP_WINDOWS
    return 0;
#else
    return NULL;
#endif
}

static int bg_thread_start_impl(void)
{
#ifdef EDP_WINDOWS
    sBG.hThread = CreateThread(NULL, 0, bg_thread_proc, NULL, 0, NULL);
    
    if (!sBG.hThread) return ERR_CouldNotInit;
#else
    if (pthread_create(&sBG.pthread, NULL, bg_thread_proc, NULL)) return ERR_CouldNotInit;
#endif
    
    return ERR_None;
}

int bg_thread_start(void)
{
    int rc;
    
    array_init(&sBG.patches, ManifestEntry);
    tbl_init(&sBG.patchesByName, uint32_t);
    
    sBG.input   = ringbuf_create();
    sBG.output  = ringbuf_create();
    
    if (!sBG.input || !sBG.output) return ERR_OutOfMemory;
    
    rc = semaphore_init(&sBG.semaphore);
    
    if (rc) return rc;
    
    return bg_thread_start_impl();
}

void bg_thread_stop(void)
{
    RingPacket rp;
    
    ring_packet_init(&rp, RingOp_Terminate, 0, NULL);
    
    for (;;)
    {
        if (!bg_thread_send(&rp))
            break;
    }
    
#ifdef EDP_WINDOWS
    WaitForSingleObject(sBG.hThread, INFINITE);
    CloseHandle(sBG.hThread);
#else
    pthread_join(sBG.pthread, NULL);
#endif
    
    db_deinit(sBG.db);
    array_deinit(&sBG.patches, parse_deinit_each_patch_entry);
    tbl_deinit(&sBG.patchesByName, NULL);

    ringbuf_destroy(sBG.input);
    ringbuf_destroy(sBG.output);
    semaphore_deinit(&sBG.semaphore);
}

int bg_thread_send(const RingPacket* rp)
{
    int rc = ringbuf_push(sBG.input, rp);
    return (rc) ? rc : semaphore_trigger(&sBG.semaphore);
}

int bg_thread_recv(RingPacket* rp)
{
    return ringbuf_pop(sBG.output, rp);
}
