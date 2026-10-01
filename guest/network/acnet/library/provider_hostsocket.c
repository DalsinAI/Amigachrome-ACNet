/* ACNet's HostSocket provider: the Amiga's sockets carried out by the Linux
 * host, through acnet.device and the ACNet card. Each opener opens the
 * device and registers one waiter (its task and a signal of its own); the
 * device's interrupt signals it while armed. BSD-3-Clause. */
#include <exec/memory.h>
#include <exec/errors.h>
#include <proto/exec.h>

#include "provider.h"

#define ENETDOWN 50
#define ENOMEM   12

/* acnet.device's private vectors (include/acnet_device.h). */
static inline LONG dev_call(struct Library *dev, struct ACNCall *c)
{
    register LONG d0 __asm("d0");
    register struct ACNCall *a0 __asm("a0") = c;
    register struct Library *a6 __asm("a6") = dev;
    __asm volatile ("jsr -42(%%a6)" : "=r"(d0), "+r"(a0) : "r"(a6) : "d1", "a1", "cc", "memory");
    return d0;
}
static inline void dev_waiter(struct Library *dev, struct ACNWaiter *w, int add)
{
    register struct ACNWaiter *a0 __asm("a0") = w;
    register struct Library *a6 __asm("a6") = dev;
    if (add) __asm volatile ("jsr -48(%%a6)" : "+r"(a0) : "r"(a6) : "d0", "d1", "a1", "cc", "memory");
    else     __asm volatile ("jsr -54(%%a6)" : "+r"(a0) : "r"(a6) : "d0", "d1", "a1", "cc", "memory");
}

LONG prov_open(struct SocketBase *sb)
{
    sb->dev_io = AllocMem(sizeof(struct IOStdReq), MEMF_PUBLIC | MEMF_CLEAR);
    if (!sb->dev_io) return ENOMEM;
    if (OpenDevice((STRPTR)ACNET_DEVICE_NAME, 0, (struct IORequest *)sb->dev_io, 0)) {
        FreeMem(sb->dev_io, sizeof(struct IOStdReq));
        sb->dev_io = NULL;
        return ENETDOWN;
    }
    sb->dev = (struct Library *)sb->dev_io->io_Device;
    sb->sigbit = AllocSignal(-1);
    if (sb->sigbit < 0) { prov_close(sb); return ENOMEM; }
    sb->waiter.task = sb->owner;
    sb->waiter.sigmask = 1UL << sb->sigbit;
    sb->waiter.waiting = 0;
    dev_waiter(sb->dev, &sb->waiter, 1);
    return 0;
}

void prov_close(struct SocketBase *sb)
{
    if (sb->dev && sb->sigbit >= 0) dev_waiter(sb->dev, &sb->waiter, 0);
    if (sb->sigbit >= 0) FreeSignal(sb->sigbit);
    sb->sigbit = -1;
    if (sb->dev_io) {
        CloseDevice((struct IORequest *)sb->dev_io);
        FreeMem(sb->dev_io, sizeof(struct IOStdReq));
    }
    sb->dev_io = NULL;
    sb->dev = NULL;
}

LONG prov_call(struct SocketBase *sb, ULONG cmd, ULONG a0, ULONG a1, ULONG a2, ULONG a3,
               const void *tx, ULONG txlen, void *rx, ULONG rxmax, ULONG *rxlen)
{
    struct ACNCall c;
    c.cmd = cmd; c.arg[0] = a0; c.arg[1] = a1; c.arg[2] = a2; c.arg[3] = a3;
    c.tx = tx; c.txlen = txlen; c.rx = rx; c.rxmax = rxmax;
    c.result = -1; c.err = ENETDOWN; c.rxlen = 0;
    if (sb->dev) dev_call(sb->dev, &c);
    sb->last_err = c.err;
    if (rxlen) *rxlen = c.rxlen;
    return c.result;
}

void prov_arm(struct SocketBase *sb)
{
    sb->waiter.task = FindTask(NULL);
    sb->waiter.waiting = 1;
}

void prov_disarm(struct SocketBase *sb)
{
    sb->waiter.waiting = 0;
}
