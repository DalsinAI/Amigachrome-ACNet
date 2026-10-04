/* ACNet bsdsocket.library: errno and h_errno, and where an opener wants
 * them. MIT. */
#include <exec/types.h>
#include <proto/exec.h>

#include "lib_internal.h"

void set_errno(struct SocketBase *sb, LONG e)
{
    sb->errno_val = e;
    if (!sb->errno_ptr) return;
    if (sb->errno_size == 1) *(UBYTE *)sb->errno_ptr = (UBYTE)e;
    else if (sb->errno_size == 2) *(UWORD *)sb->errno_ptr = (UWORD)e;
    else *(ULONG *)sb->errno_ptr = (ULONG)e;
}

LONG fail(struct SocketBase *sb, LONG e)
{
    set_errno(sb, e);
    return -1;
}

LONG fail_provider(struct SocketBase *sb)
{
    return fail(sb, sb->last_err ? (LONG)sb->last_err : AE_NETDOWN);
}

void set_herrno(struct SocketBase *sb, LONG e)
{
    sb->herrno_val = e;
    if (sb->herrno_ptr) *sb->herrno_ptr = e;
}

LONG bsd_Errno(struct SocketBase *sb) { return sb->errno_val; }

VOID bsd_SetErrnoPtr(struct SocketBase *sb, APTR errno_ptr, LONG size)
{
    if (size == 1 || size == 2 || size == 4) {
        sb->errno_ptr = errno_ptr;
        sb->errno_size = size;
    } else set_errno(sb, AE_INVAL);
}

VOID bsd_SetSocketSignals(struct SocketBase *sb, ULONG int_mask, ULONG io_mask, ULONG urgent_mask)
{
    sb->sigintr = int_mask;
    sb->sigio = io_mask;
    sb->sigurg = urgent_mask;
}

/* No asynchronous socket events in version 1: never any pending. */
LONG bsd_GetSocketEvents(struct SocketBase *sb, ULONG *event_ptr)
{
    (void)sb;
    if (event_ptr) *event_ptr = 0;
    return -1;
}

/* syslog() goes nowhere in version 1. */
VOID bsd_vsyslog(struct SocketBase *sb, LONG pri, STRPTR msg, APTR args)
{
    (void)sb; (void)pri; (void)msg; (void)args;
}
