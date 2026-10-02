/* Read-only Roadshow route-info compatibility for ACTCPTools arp.
 * This is a compatibility profile, not part of the ACNet socket core. */
#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <sys/socket.h>
#include <net/route.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <netinet/if_ether.h>

#include <acnetwork.h>
#include "../../library/lib_internal.h"

#define ACNET_ROUTE_NEIGH_MAX 64

static ULONG route_bytes_for(LONG count)
{
    ULONG one = sizeof(struct rt_msghdr) + sizeof(struct sockaddr_inarp) + sizeof(struct sockaddr_dl);
    return (ULONG)count * one + sizeof(struct rt_msghdr);
}

static APTR route_alloc(ULONG bytes)
{
    ULONG *raw = AllocMem(sizeof(ULONG) + bytes, MEMF_PUBLIC | MEMF_CLEAR);
    if (!raw) return NULL;
    raw[0] = bytes;
    return raw + 1;
}

VOID bsd_FreeRouteInfo(struct SocketBase *sb, struct rt_msghdr *buf)
{
    ULONG *raw;
    ULONG bytes;
    (void)sb;
    if (!buf) return;
    raw = ((ULONG *)buf) - 1;
    bytes = raw[0];
    FreeMem(raw, sizeof(ULONG) + bytes);
}

struct rt_msghdr *bsd_GetRouteInfo(struct SocketBase *sb, LONG address_family, LONG flags)
{
    struct ACNetworkNeighbour *neighbors;
    struct rt_msghdr *table;
    UBYTE *cursor;
    ULONG rxlen = 0;
    ULONG one;
    LONG count, i;

    if (address_family != AF_INET || flags != RTF_LLINFO) {
        set_errno(sb, AE_OPNOTSUPP);
        return NULL;
    }

    neighbors = AllocMem(ACNET_ROUTE_NEIGH_MAX * sizeof(*neighbors), MEMF_PUBLIC | MEMF_CLEAR);
    if (!neighbors) { set_errno(sb, AE_NOMEM); return NULL; }

    count = prov_call(sb, ACNETWORK_CMD_NEIGHBOURS, ACNET_ROUTE_NEIGH_MAX, 0, 0, 0,
                      NULL, 0, neighbors,
                      ACNET_ROUTE_NEIGH_MAX * sizeof(*neighbors), &rxlen);
    if (count < 0) {
        FreeMem(neighbors, ACNET_ROUTE_NEIGH_MAX * sizeof(*neighbors));
        fail_provider(sb);
        return NULL;
    }
    if ((ULONG)count > rxlen / sizeof(*neighbors))
        count = rxlen / sizeof(*neighbors);

    one = sizeof(struct rt_msghdr) + sizeof(struct sockaddr_inarp) + sizeof(struct sockaddr_dl);
    table = route_alloc(route_bytes_for(count));
    if (!table) {
        FreeMem(neighbors, ACNET_ROUTE_NEIGH_MAX * sizeof(*neighbors));
        set_errno(sb, AE_NOMEM);
        return NULL;
    }

    cursor = (UBYTE *)table;
    for (i = 0; i < count; ++i) {
        struct ACNetworkNeighbour *n = &neighbors[i];
        struct rt_msghdr *rtm = (struct rt_msghdr *)cursor;
        struct sockaddr_inarp *sin = (struct sockaddr_inarp *)(rtm + 1);
        struct sockaddr_dl *sdl = (struct sockaddr_dl *)(sin + 1);

        rtm->rtm_msglen = (UWORD)one;
        rtm->rtm_version = RTM_VERSION;
        rtm->rtm_type = RTM_GET;
        rtm->rtm_index = (UWORD)n->ifindex;
        rtm->rtm_flags = RTF_UP | RTF_HOST | RTF_LLINFO;
        rtm->rtm_addrs = RTA_DST | RTA_GATEWAY;
        rtm->rtm_rmx.rmx_expire = 1;  /* dynamic neighbour; do not print permanent */

        sin->sin_len = sizeof(*sin);
        sin->sin_family = AF_INET;
        CopyMem(n->ipv4, &sin->sin_addr.s_addr, 4);

        sdl->sdl_len = sizeof(*sdl);
        sdl->sdl_family = AF_LINK;
        sdl->sdl_index = (UWORD)n->ifindex;
        sdl->sdl_type = IFT_ETHER;
        sdl->sdl_nlen = 0;
        sdl->sdl_alen = (n->state & ACNETWORK_NEIGH_VALID) ? 6 : 0;
        if (sdl->sdl_alen) CopyMem(n->mac, sdl->sdl_data, 6);

        cursor += one;
    }

    FreeMem(neighbors, ACNET_ROUTE_NEIGH_MAX * sizeof(*neighbors));
    set_errno(sb, 0);
    return table;
}
