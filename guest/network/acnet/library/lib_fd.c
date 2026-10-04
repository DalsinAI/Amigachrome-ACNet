/* ACNet bsdsocket.library: each opener's descriptor table. A descriptor
 * names a host socket handle; Dup2Socket and ReleaseCopyOfSocket let two
 * descriptors share one, so handles are counted and the host socket closes
 * with the last. ReleaseSocket and ObtainSocket pass a socket from one
 * opener to another (a server handing a connection to a worker task).
 * MIT. */
#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include "lib_internal.h"

/* Shared by all openers: the handle counts and the sockets waiting to be
 * obtained. Changed under Forbid(). */
static struct { LONG handle, refs; } refs[DTABLE_MAX];
static struct { LONG id, handle; UBYTE type, used, pad[2]; } released[16];
static LONG next_id = 1;

void handle_ref(LONG handle)
{
    LONG i, empty = -1;
    Forbid();
    for (i = 0; i < DTABLE_MAX; i++) {
        if (refs[i].handle == handle) { refs[i].refs++; Permit(); return; }
        if (!refs[i].handle && empty < 0) empty = i;
    }
    if (empty >= 0) { refs[empty].handle = handle; refs[empty].refs = 1; }
    Permit();
}

/* The references left on handle after dropping one. */
static LONG handle_unref(LONG handle)
{
    LONG i, left = 0;
    Forbid();
    for (i = 0; i < DTABLE_MAX; i++)
        if (refs[i].handle == handle) {
            left = --refs[i].refs;
            if (left <= 0) { refs[i].handle = 0; left = 0; }
            break;
        }
    Permit();
    return left;
}

struct FD *fd_get(struct SocketBase *sb, LONG s)
{
    if (s < 0 || s >= sb->dtablesize || !sb->fds[s].handle) { set_errno(sb, AE_BADF); return NULL; }
    return &sb->fds[s];
}

LONG fd_alloc(struct SocketBase *sb, LONG handle, UBYTE type, LONG at)
{
    LONG s = at;
    if (s < 0)
        for (s = 0; s < sb->dtablesize && sb->fds[s].handle; s++) ;
    if (s >= sb->dtablesize) return fail(sb, AE_MFILE);
    struct FD *f = &sb->fds[s];
    f->handle = handle;
    f->type = type;
    f->nonblock = f->async = 0;
    f->rcvtimeo.tv_secs = f->rcvtimeo.tv_micro = 0;
    f->sndtimeo.tv_secs = f->sndtimeo.tv_micro = 0;
    f->eventmask = 0;
    return s;
}

void fd_free(struct SocketBase *sb, LONG s)
{
    LONG h = sb->fds[s].handle;
    sb->fds[s].handle = 0;
    if (h && handle_unref(h) == 0)
        prov_call(sb, ACHS_CMD_CLOSE, (ULONG)h, 0, 0, 0, NULL, 0, NULL, 0, NULL);
}

UBYTE *scratch(struct SocketBase *sb)
{
    if (!sb->scratch) sb->scratch = AllocMem(SCRATCH_SIZE, MEMF_PUBLIC);
    if (!sb->scratch) set_errno(sb, AE_NOMEM);
    return sb->scratch;
}

/* ---- descriptors in the interface ------------------------------------------------ */

LONG bsd_getdtablesize(struct SocketBase *sb) { return sb->dtablesize; }

LONG bsd_CloseSocket(struct SocketBase *sb, LONG sock)
{
    if (!fd_get(sb, sock)) return -1;
    fd_free(sb, sock);
    return 0;
}

LONG bsd_Dup2Socket(struct SocketBase *sb, LONG old_socket, LONG new_socket)
{
    struct FD *f = fd_get(sb, old_socket);
    if (!f) return -1;
    if (new_socket == old_socket) return new_socket;
    if (new_socket >= sb->dtablesize) return fail(sb, AE_BADF);
    if (new_socket >= 0 && sb->fds[new_socket].handle) fd_free(sb, new_socket);
    struct FD copy = *f;
    LONG s = fd_alloc(sb, copy.handle, copy.type, new_socket);
    if (s < 0) return -1;
    handle_ref(copy.handle);
    sb->fds[s] = copy;
    return s;
}

static LONG release(struct SocketBase *sb, LONG sock, LONG id, int keep)
{
    struct FD *f = fd_get(sb, sock);
    if (!f) return -1;
    LONG i, slot = -1;
    Forbid();
    if (id == A_UNIQUE_ID) id = next_id++;
    for (i = 0; i < 16; i++) {
        if (released[i].used && released[i].id == id) { Permit(); return fail(sb, AE_INVAL); }
        if (!released[i].used && slot < 0) slot = i;
    }
    if (slot < 0) { Permit(); return fail(sb, AE_MFILE); }
    released[slot].id = id;
    released[slot].handle = f->handle;
    released[slot].type = f->type;
    released[slot].used = 1;
    Permit();
    if (keep) handle_ref(f->handle);             /* both the opener and the release hold it */
    else f->handle = 0;                          /* the reference moves with the socket */
    return id;
}

LONG bsd_ReleaseSocket(struct SocketBase *sb, LONG sock, LONG id) { return release(sb, sock, id, 0); }
LONG bsd_ReleaseCopyOfSocket(struct SocketBase *sb, LONG sock, LONG id) { return release(sb, sock, id, 1); }

LONG bsd_ObtainSocket(struct SocketBase *sb, LONG id, LONG domain, LONG type, LONG protocol)
{
    (void)domain; (void)protocol;
    LONG i;
    Forbid();
    for (i = 0; i < 16; i++)
        if (released[i].used && released[i].id == id && (!type || released[i].type == type)) {
            LONG handle = released[i].handle;
            UBYTE t = released[i].type;
            released[i].used = 0;
            Permit();
            LONG s = fd_alloc(sb, handle, t, -1);
            if (s < 0) { if (handle_unref(handle) == 0) prov_call(sb, ACHS_CMD_CLOSE, (ULONG)handle, 0, 0, 0, NULL, 0, NULL, 0, NULL); }
            return s;
        }
    Permit();
    return fail(sb, AE_INVAL);
}
