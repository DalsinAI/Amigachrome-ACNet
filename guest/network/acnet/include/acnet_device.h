#ifndef ACNET_DEVICE_H
#define ACNET_DEVICE_H

/*
 * acnet.device's private calls: the one driver for the ACNet card (Zorro II,
 * Dalsin product 6; common/protocol/achostsocket.h). ACNet's bsdsocket.library
 * opens the device and calls these vectors directly, with no I/O request per
 * call. Design: docs/architecture/ACNET_DESIGN.md.
 *
 * The standard device vectors come first (Open -6, Close -12, Expunge -18,
 * reserved -24, BeginIO -30, AbortIO -36). BeginIO answers IOERR_NOCMD until
 * the SANA-II side exists.
 */
#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/tasks.h>

#define ACNET_DEVICE_NAME   "acnet.device"
#define ACNET_DEVICE_VERSION 1

/* One HostSocket command. The device copies tx in, runs the command and
 * copies up to rxmax bytes of the answer out, all under Forbid(). */
struct ACNCall {
    ULONG cmd;
    ULONG arg[4];
    const UBYTE *tx;
    ULONG txlen;
    UBYTE *rx;
    ULONG rxmax;
    LONG  result;      /* RESULT: the call's value, -1 on failure */
    ULONG err;         /* ERRNO: the Amiga errno (h_errno for a failed name answer) */
    ULONG rxlen;       /* the bytes the host left (may exceed rxmax: the rest is not copied) */
};

/* A task woken (Signal) on every card event while it is waiting. The library
 * sets task and sigmask, then waiting = 1 before it polls and 0 after Wait(). */
struct ACNWaiter {
    struct MinNode node;
    struct Task *task;
    ULONG sigmask;
    volatile UBYTE waiting;
    UBYTE pad[3];
};

#define ACN_STATE_CARD      0x01   /* the card is fitted and answers */
#define ACN_STATE_ONLINE    0x02   /* the instance's Network switch is on */

/* LVOs after the standard six. */
#define ACN_LVO_CALL        (-42)
#define ACN_LVO_ADDWAITER   (-48)
#define ACN_LVO_REMWAITER   (-54)
#define ACN_LVO_STATE       (-60)

#endif
