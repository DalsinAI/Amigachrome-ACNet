/* ACNet bsdsocket.library: SocketBaseTagList, an opener's settings by tag
 * (libraries/bsdsocket.h). BSD-3-Clause. */
#include <exec/types.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <libraries/bsdsocket.h>

#include "lib_internal.h"

static const char release[] = "ACNet 1.0 (AmigaChrome)";

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
    ULONG tag = ti->ti_Tag, code = SBTM_CODE(tag);
    int set = tag & SBTF_SET, ref = tag & SBTF_REF;
    ULONG *where = ref ? (ULONG *)ti->ti_Data : (ULONG *)&ti->ti_Data;
    ULONG v = ref && set ? *where : ti->ti_Data;
#define GIVE(x) do { *where = (ULONG)(x); return 0; } while (0)
    switch (code) {
    case SBTC_BREAKMASK:   if (set) { sb->sigintr = v; return 0; } GIVE(sb->sigintr);
    case SBTC_SIGIOMASK:   if (set) { sb->sigio = v; return 0; } GIVE(sb->sigio);
    case SBTC_SIGURGMASK:  if (set) { sb->sigurg = v; return 0; } GIVE(sb->sigurg);
    case SBTC_SIGEVENTMASK: if (set) { sb->sigevent = v; return 0; } GIVE(sb->sigevent);
    case SBTC_ERRNO:       if (set) { set_errno(sb, (LONG)v); return 0; } GIVE(sb->errno_val);
    case SBTC_HERRNO:      if (set) { set_herrno(sb, (LONG)v); return 0; } GIVE(sb->herrno_val);
    case SBTC_DTABLESIZE:  if (set) return set_dtablesize(sb, (LONG)v); GIVE(sb->dtablesize);
    case SBTC_ERRNOBYTEPTR: if (!set) return -1; sb->errno_ptr = (APTR)v; sb->errno_size = 1; return 0;
    case SBTC_ERRNOWORDPTR: if (!set) return -1; sb->errno_ptr = (APTR)v; sb->errno_size = 2; return 0;
    case SBTC_ERRNOLONGPTR: if (!set) return -1; sb->errno_ptr = (APTR)v; sb->errno_size = 4; return 0;
    case SBTC_HERRNOLONGPTR: if (!set) return -1; sb->herrno_ptr = (LONG *)v; return 0;
    case SBTC_ERRNOSTRPTR: if (set) return -1; GIVE(errno_string((LONG)*where));
    case SBTC_HERRNOSTRPTR: if (set) return -1; GIVE(herrno_string((LONG)*where));
    case SBTC_RELEASESTRPTR: if (set) return -1; GIVE(release);
    case SBTC_LOGSTAT: case SBTC_LOGTAGPTR: case SBTC_LOGFACILITY: case SBTC_LOGMASK:
        if (set) return 0; GIVE(0);                  /* no syslog in version 1 */
    case SBTC_CAN_SHARE_LIBRARY_BASES: case SBTC_HAVE_ROUTING_API: case SBTC_HAVE_INTERFACE_API:
    case SBTC_HAVE_MONITORING_API: case SBTC_HAVE_STATUS_API: case SBTC_HAVE_DNS_API:
    case SBTC_HAVE_LOCAL_DATABASE_API: case SBTC_HAVE_KERNEL_MEMORY_API: case SBTC_HAVE_SERVER_API:
    case SBTC_HAVE_ROADSHOWDATA_API: case SBTC_HAVE_GETHOSTADDR_R_API:
        if (set) return -1; GIVE(FALSE);
    case SBTC_HAVE_ADDRESS_CONVERSION_API:
        if (set) return -1; GIVE(TRUE);              /* inet_aton, inet_ntop, inet_pton */
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
