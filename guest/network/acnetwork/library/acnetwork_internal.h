#ifndef ACNETWORK_INTERNAL_H
#define ACNETWORK_INTERNAL_H

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/io.h>
#include <exec/tasks.h>
#include <exec/devices.h>
#include <dos/dos.h>

#include <acnetwork.h>
#include "acnet_device.h"

#define REG(r, decl) register decl __asm(#r)

struct ACNetworkBase {
    struct Library lib;
    UWORD pad;
    struct ACNetworkBase *master;
    BPTR seglist;
    struct Task *owner;
    struct IOStdReq *dev_io;
    struct Library *dev;
    struct ACNWaiter waiter;
    BYTE sigbit;
    UBYTE pad2[3];
    ULONG last_error;
};

extern struct ExecBase *SysBase;

#endif
