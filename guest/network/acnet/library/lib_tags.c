/* ACNet bsdsocket.library: classic per-opener SocketBaseTagList settings.
 * BSD-3-Clause. */
#include <exec/types.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <proto/exec.h>

#include "lib_internal.h"
#include "acnet_socket_tags.h"

/* A bigger descriptor table, keeping the open sockets. */
static LONG set_dtablesize(struct SocketBase *sb, LONG n)
{
    if (n < sb->dtablesize) return 0;               /* never shrinks */
    if (n > DTABLE_MAX) n = DTABLE_MAX;
    struct FD *fds = AllocMem(n * sizeof(struct FD), MEMF_PUBLIC | MEMF_CLEAR);
    if (!fds) return -1;
    CopyMem(sb->fds, fds, sb->dtablesize * sizeof(struct FD));
    FreeMem(sb->fds, sb->dtablesize * sizeof(struct FD));
    sb->fds = fds;
    sb->dtablesize = n;
    return 0;
}

/* One tag: 0, or -1 when ACNet does not know or allow it. */
static LONG one_tag(struct SocketBase *sb, struct TagItem *ti)
{
    ULONG tag = ti->ti_Tag, code = ACSBTM_CODE(tag);
    int set = tag & ACSBTF_SET, ref = tag & ACSBTF_REF;
    ULONG *where = ref ? (ULONG *)ti->ti_Data : (ULONG *)&ti->ti_Data;
    ULONG v = ref && set ? *where : ti->ti_Data;
#define GIVE(x) do { *where = (ULONG)(x); return 0; } while (0)
    switch (code) {
    case ACSBTC_BREAKMASK:   if (set) { sb->sigintr = v; return 0; } GIVE(sb->sigintr);
    case ACSBTC_SIGIOMASK:   if (set) { sb->sigio = v; return 0; } GIVE(sb->sigio);
    case ACSBTC_SIGURGMASK:  if (set) { sb->sigurg = v; return 0; } GIVE(sb->sigurg);
    case ACSBTC_SIGEVENTMASK: if (set) { sb->sigevent = v; return 0; } GIVE(sb->sigevent);
    case ACSBTC_ERRNO:       if (set) { set_errno(sb, (LONG)v); return 0; } GIVE(sb->errno_val);
    case ACSBTC_HERRNO:      if (set) { set_herrno(sb, (LONG)v); return 0; } GIVE(sb->herrno_val);
    case ACSBTC_DTABLESIZE:  if (set) return set_dtablesize(sb, (LONG)v); GIVE(sb->dtablesize);
    case ACSBTC_ERRNOBYTEPTR: if (!set) return -1; sb->errno_ptr = (APTR)v; sb->errno_size = 1; return 0;
    case ACSBTC_ERRNOWORDPTR: if (!set) return -1; sb->errno_ptr = (APTR)v; sb->errno_size = 2; return 0;
    case ACSBTC_ERRNOLONGPTR: if (!set) return -1; sb->errno_ptr = (APTR)v; sb->errno_size = 4; return 0;
    case ACSBTC_HERRNOLONGPTR: if (!set) return -1; sb->herrno_ptr = (LONG *)v; return 0;
    case ACSBTC_HAVE_ROUTING_API: if (set) return -1; GIVE(1);  /* read-only GetRouteInfo compatibility */
    case ACSBTC_ERRNOSTRPTR: if (set) return -1; GIVE(errno_string((LONG)*where));
    case ACSBTC_HERRNOSTRPTR: if (set) return -1; GIVE(herrno_string((LONG)*where));
    case ACSBTC_LOGSTAT: case ACSBTC_LOGTAGPTR: case ACSBTC_LOGFACILITY: case ACSBTC_LOGMASK:
        if (set) return 0; GIVE(0);                  /* no syslog in version 1 */
    case ACSBTC_FDCALLBACK: case ACSBTC_IOERRNOSTRPTR: case ACSBTC_S2ERRNOSTRPTR: case ACSBTC_S2WERRNOSTRPTR:
        return -1;
    }
#undef GIVE
    return -1;
}

/* 0, or the number of the first tag that failed. */
LONG bsd_SocketBaseTagList(struct SocketBase *sb, struct TagItem *tags)
{
    LONG n = 0;
    struct TagItem *ti = tags;
    while (ti) {
        switch (ti->ti_Tag) {
        case TAG_DONE: return 0;
        case TAG_IGNORE: ti++; continue;
        case TAG_MORE: ti = (struct TagItem *)ti->ti_Data; continue;
        case TAG_SKIP: ti += ti->ti_Data + 1; continue;
        }
        n++;
        if (one_tag(sb, ti)) return n;
        ti++;
    }
    return 0;
}
