
#include "bg_thread.h"

typedef struct BG {
    sqlite3*        db;
    Array           patches;
    HashTbl         patchesByName;
    SimpleString*   eqPath;
    RingBuf*        input;
    RingBuf*        output;
    Semaphore       semaphore;
#ifdef EDP_WINDOWS
    HANDLE          hThread;
#else
    pthread_t       pthread;
#endif
} BG;

static BG sBG;

static void bg_thread_init_db(void)
{
    int rc;
    sqlite3_stmt* stmt;
    RingPacket rp;
    
    rc = db_init(&sBG.db);
    
    if (rc)
    {
    open_err:
        ring_packet_init_value(&rp, RingOp_CouldNotOpenDB, rc);
        ringbuf_push(sBG.output, &rp);
        return;
    }
    
    stmt = db_prep_literal(sBG.db, "SELECT value FROM info WHERE key = 'eqpath'");
    
    if (!stmt) goto open_err;
    
    for (;;)
    {
        const char* str;
        uint32_t len;
        
        rc = sqlite3_step(stmt);
        
        if (rc == SQLITE_BUSY || rc == SQLITE_LOCKED)
            continue;
        
        if (rc == SQLITE_DONE)
            break;
        
        if (rc != SQLITE_ROW)
            goto stmt_err;
        
        len = sqlite3_column_bytes(stmt, 0);
        str = (const char*)sqlite3_column_text(stmt, 0);
        
        if (str && len)
        {
            sBG.eqPath = sstr_create(str, len);
            
            if (!sBG.eqPath)
                goto stmt_err;
        }
    }
    
    sqlite3_finalize(stmt);
    
    if (!sBG.eqPath)
    {
        ring_packet_init_op(&rp, RingOp_NeedEqPath);
        ringbuf_push(sBG.output, &rp);
    }
    
    return;
    
stmt_err:
    sqlite3_finalize(stmt);
    goto open_err;
}

#ifdef EDP_WINDOWS
static DWORD WINAPI
#else
static void*
#endif
bg_thread_proc(void* unused)
{
    (void)unused;
    
    bg_thread_init_db();
    
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
    
    if (sBG.db)
    {
        db_deinit(sBG.db);
        sBG.db = NULL;
    }
    
    if (sBG.eqPath)
    {
        sstr_destroy(sBG.eqPath);
        sBG.eqPath = NULL;
    }
    
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
