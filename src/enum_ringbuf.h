
#ifndef ENUM_RINGBUF_H
#define ENUM_RINGBUF_H

enum RingBufOp
{
    RingOp_None,
    RingOp_Terminate,
    RingOp_CouldNotOpenDB,
    RingOp_NeedEqPath,
    RingOp_PatchDownloadFailed,
    RingOp_COUNT
};

#endif/*ENUM_RINGBUF_H*/
