/* acnet.device: the one driver for ACNet, AmigaChrome's network card
 * (Zorro II, Dalsin product 6; common/protocol/achostsocket.h).
 *
 * It finds the card through expansion.library and claims it, says hello to
 * the host (dropping the last boot's sockets), adds one server to the INT2
 * chain and turns the card's interrupt on. ACNet's bsdsocket.library calls
 * its private vectors (include/acnet_device.h): a command, run under
 * Forbid() with the card's one set of arguments; and waiters, tasks the
 * interrupt signals on each card event. Without a card the device still
 * opens and every command fails ENETDOWN.
 *
 * BeginIO answers IOERR_NOCMD until the SANA-II side exists. Built bare by
 * ../build.sh (bebbo's m68k-amigaos-gcc, NDK 3.2); design
 * docs/architecture/ACNET_DESIGN.md. BSD-3-Clause.
 */
#include <exec/types.h>
#include <exec/resident.h>
#include <exec/devices.h>
#include <exec/errors.h>
#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <hardware/intbits.h>
#include <libraries/configvars.h>
#include <proto/exec.h>
#include <proto/expansion.h>

#include "achostsocket.h"
#include "acnet_device.h"

#define REG(r, decl) register decl __asm(#r)
#define ENETDOWN 50

struct ACNetBase {
    struct Library lib;
    UWORD pad;
    BPTR seglist;
    struct ConfigDev *cd;
    volatile UBYTE *card;          /* the card's base, or NULL */
    ULONG state;                   /* ACN_STATE_* from the last hello */
    UBYTE started, irq_added;
    struct Interrupt irq;
    struct MinList waiters;        /* struct ACNWaiter */
};

struct ExecBase *SysBase;

/* Run as a program, the device does nothing. This must stay the first code
 * in the file: build.sh keeps source order (-fno-toplevel-reorder). */
int start(void) { return -1; }

static const char dev_name[] = ACNET_DEVICE_NAME;
static const char dev_id[] = "acnet.device 1.0 (1.10.2026) AmigaChrome ACNet\r\n";
static const char ver[] __attribute__((used)) = "$VER: acnet.device 1.0 (1.10.2026) AmigaChrome ACNet";

#define R(off) (*(volatile ULONG *)(b->card + (off)))

/* ---- the card ----------------------------------------------------------------- */

/* Copies to and from the card a long at a time (its buffers are long
 * aligned): never CopyMem, which a system patch may do with MOVE16. */
static void to_card(volatile UBYTE *dst, const UBYTE *src, ULONG n)
{
    volatile ULONG *d = (volatile ULONG *)dst;
    while (n >= 4) { *d++ = (ULONG)src[0] << 24 | (ULONG)src[1] << 16 | (ULONG)src[2] << 8 | src[3]; src += 4; n -= 4; }
    volatile UBYTE *db = (volatile UBYTE *)d;
    while (n--) *db++ = *src++;
}
static void from_card(UBYTE *dst, volatile const UBYTE *src, ULONG n)
{
    volatile const ULONG *s = (volatile const ULONG *)src;
    while (n >= 4) { ULONG v = *s++; dst[0] = (UBYTE)(v >> 24); dst[1] = (UBYTE)(v >> 16); dst[2] = (UBYTE)(v >> 8); dst[3] = (UBYTE)v; dst += 4; n -= 4; }
    volatile const UBYTE *sb = (volatile const UBYTE *)s;
    while (n--) *dst++ = *sb++;
}

static LONG call(struct ACNetBase *b, struct ACNCall *c)
{
    if (!b->card) { c->result = -1; c->err = ENETDOWN; c->rxlen = 0; return -1; }
    ULONG tl = c->txlen > ACHS_TX_SIZE ? ACHS_TX_SIZE : c->txlen;
    Forbid();
    R(ACHS_REG_ARG0) = c->arg[0];
    R(ACHS_REG_ARG1) = c->arg[1];
    R(ACHS_REG_ARG2) = c->arg[2];
    R(ACHS_REG_ARG3) = c->arg[3];
    R(ACHS_REG_TXLEN) = tl;
    if (tl) to_card(b->card + ACHS_TX, c->tx, tl);
    R(ACHS_REG_COMMAND) = c->cmd;                 /* the host runs it now */
    c->result = (LONG)R(ACHS_REG_RESULT);
    c->err = R(ACHS_REG_ERRNO);
    c->rxlen = R(ACHS_REG_RXLEN);
    ULONG n = c->rxlen < c->rxmax ? c->rxlen : c->rxmax;
    if (n > ACHS_RX_SIZE) n = ACHS_RX_SIZE;
    if (n && c->rx) from_card(c->rx, b->card + ACHS_RX, n);
    Permit();
    return c->result;
}

/* The INT2 server: acknowledge the card's event and wake every waiting task.
 * It never claims the interrupt (the CIA-A shares the chain). */
ULONG irq_c(struct ACNetBase *b) __attribute__((used));
ULONG irq_c(struct ACNetBase *b)
{
    if (!b->card || !R(ACHS_REG_EVENT)) return 0;
    R(ACHS_REG_EVENT) = 0;
    struct ACNWaiter *w;
    for (w = (struct ACNWaiter *)b->waiters.mlh_Head; w->node.mln_Succ; w = (struct ACNWaiter *)w->node.mln_Succ)
        if (w->waiting) Signal(w->task, w->sigmask);
    return 0;
}
void irq_entry(void);
__asm__(
    "    .text\n"
    "    .even\n"
    "_irq_entry:\n"              /* a1 = is_Data; the chain goes on when Z is set */
    "    move.l a1,-(sp)\n"
    "    jsr _irq_c\n"
    "    addq.l #4,sp\n"
    "    tst.l d0\n"
    "    rts\n");

static void hello(struct ACNetBase *b, ULONG first)
{
    ULONG rx[2] = { 0, 0 };
    struct ACNCall c = { ACHS_CMD_HELLO, { first, 0, 0, 0 }, NULL, 0, (UBYTE *)rx, sizeof rx, 0, 0, 0 };
    call(b, &c);
    b->state = (ULONG)c.result == ACHS_VERSION && c.err == 0 && c.rxlen >= 4 ? (rx[0] & ACHS_STATE_ONLINE ? ACN_STATE_CARD | ACN_STATE_ONLINE : ACN_STATE_CARD) : 0;
}

static void start_card(struct ACNetBase *b)
{
    b->started = 1;
    struct Library *ExpansionBase = OpenLibrary("expansion.library", 37);
    if (!ExpansionBase) return;
    struct ConfigDev *cd = FindConfigDev(NULL, ACNET_MANUFACTURER, ACNET_PRODUCT);
    if (!cd) cd = FindConfigDev(NULL, ACNET_MANUFACTURER_OLD, ACNET_PRODUCT);
    CloseLibrary(ExpansionBase);
    if (!cd || !cd->cd_BoardAddr) return;
    volatile UBYTE *card = cd->cd_BoardAddr;
    if (*(volatile ULONG *)(card + ACHS_REG_SIGNATURE) != ACHS_SIGNATURE ||
        (*(volatile ULONG *)(card + ACHS_REG_VERSION) >> 16) != (ACHS_VERSION >> 16)) return;
    b->cd = cd;
    cd->cd_Flags &= ~CDF_CONFIGME;                /* claimed: ShowConfig names the driver */
    cd->cd_Driver = b;
    b->card = card;
    hello(b, 1);                                  /* the host drops the last boot's sockets */
    b->irq.is_Node.ln_Type = NT_INTERRUPT;
    b->irq.is_Node.ln_Pri = 0;
    b->irq.is_Node.ln_Name = (char *)dev_name;
    b->irq.is_Data = b;
    b->irq.is_Code = (void (*)(void))irq_entry;
    AddIntServer(INTB_PORTS, &b->irq);
    b->irq_added = 1;
    R(ACHS_REG_IRQ_ENABLE) = 1;
}

static void stop_card(struct ACNetBase *b)
{
    if (b->card) R(ACHS_REG_IRQ_ENABLE) = 0;
    if (b->irq_added) RemIntServer(INTB_PORTS, &b->irq);
    b->irq_added = 0;
    if (b->cd) { b->cd->cd_Driver = NULL; b->cd->cd_Flags |= CDF_CONFIGME; }
    b->card = NULL;
}

/* ---- device vectors ------------------------------------------------------------ */

static BPTR dev_expunge(REG(a6, struct ACNetBase *b))
{
    if (b->lib.lib_OpenCnt) { b->lib.lib_Flags |= LIBF_DELEXP; return 0; }
    stop_card(b);
    BPTR seg = b->seglist;
    Remove(&b->lib.lib_Node);
    FreeMem((UBYTE *)b - b->lib.lib_NegSize, b->lib.lib_NegSize + b->lib.lib_PosSize);
    return seg;
}

static LONG dev_open(REG(a1, struct IORequest *io), REG(d0, ULONG unit), REG(d1, ULONG flags), REG(a6, struct ACNetBase *b))
{
    (void)flags;
    b->lib.lib_OpenCnt++;
    if (unit != 0) {
        b->lib.lib_OpenCnt--;
        io->io_Error = IOERR_OPENFAIL;
        return IOERR_OPENFAIL;
    }
    if (!b->started) start_card(b);
    b->lib.lib_Flags &= ~LIBF_DELEXP;
    io->io_Device = (struct Device *)b;
    io->io_Unit = NULL;
    io->io_Error = 0;
    io->io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    return 0;
}

static BPTR dev_close(REG(a1, struct IORequest *io), REG(a6, struct ACNetBase *b))
{
    io->io_Device = (struct Device *)-1;
    io->io_Unit = (struct Unit *)-1;
    if (--b->lib.lib_OpenCnt == 0 && (b->lib.lib_Flags & LIBF_DELEXP)) return dev_expunge(b);
    return 0;
}

static ULONG dev_null(void) { return 0; }

static void dev_beginio(REG(a1, struct IORequest *io), REG(a6, struct ACNetBase *b))
{
    (void)b;
    io->io_Message.mn_Node.ln_Type = NT_MESSAGE;
    io->io_Error = IOERR_NOCMD;
    if (!(io->io_Flags & IOF_QUICK)) ReplyMsg(&io->io_Message);
}

static LONG dev_abortio(REG(a1, struct IORequest *io), REG(a6, struct ACNetBase *b)) { (void)io; (void)b; return 0; }

/* ---- private vectors (acnet_device.h) -------------------------------------------- */

static LONG acn_call(REG(a0, struct ACNCall *c), REG(a6, struct ACNetBase *b)) { return call(b, c); }

static void acn_addwaiter(REG(a0, struct ACNWaiter *w), REG(a6, struct ACNetBase *b))
{
    Disable();
    AddTail((struct List *)&b->waiters, (struct Node *)&w->node);
    Enable();
}

static void acn_remwaiter(REG(a0, struct ACNWaiter *w), REG(a6, struct ACNetBase *b))
{
    (void)b;
    Disable();
    Remove((struct Node *)&w->node);
    Enable();
}

static ULONG acn_state(REG(a6, struct ACNetBase *b))
{
    if (!b->card) return 0;
    hello(b, 0);
    return b->state;
}

/* ---- the ROMTag --------------------------------------------------------------------- */

static struct ACNetBase *dev_init(REG(d0, struct ACNetBase *b), REG(a0, BPTR seglist), REG(a6, struct ExecBase *sys))
{
    SysBase = sys;
    b->seglist = seglist;
    b->lib.lib_Node.ln_Type = NT_DEVICE;
    b->lib.lib_Node.ln_Name = (char *)dev_name;
    b->lib.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
    b->lib.lib_Version = ACNET_DEVICE_VERSION;
    b->lib.lib_Revision = 0;
    b->lib.lib_IdString = (APTR)dev_id;
    b->waiters.mlh_Head = (struct MinNode *)&b->waiters.mlh_Tail;
    b->waiters.mlh_Tail = NULL;
    b->waiters.mlh_TailPred = (struct MinNode *)&b->waiters.mlh_Head;
    return b;
}

static const APTR dev_vectors[] = {
    (APTR)dev_open, (APTR)dev_close, (APTR)dev_expunge, (APTR)dev_null,
    (APTR)dev_beginio, (APTR)dev_abortio,
    (APTR)acn_call, (APTR)acn_addwaiter, (APTR)acn_remwaiter, (APTR)acn_state,
    (APTR)-1
};
static const struct { ULONG size; const APTR *vectors; APTR data; APTR init; } dev_inittable = {
    sizeof(struct ACNetBase), dev_vectors, NULL, (APTR)dev_init,
};
const struct Resident dev_romtag = {
    RTC_MATCHWORD, (struct Resident *)&dev_romtag, (APTR)(&dev_romtag + 1),
    RTF_AUTOINIT, ACNET_DEVICE_VERSION, NT_DEVICE, 0, (char *)dev_name, (char *)dev_id, (APTR)&dev_inittable,
};
