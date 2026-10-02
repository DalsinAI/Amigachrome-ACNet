#ifndef ACNETWORK_H
#define ACNETWORK_H

#include <exec/types.h>

#define ACNETWORK_LIBRARY_NAME "acnetwork.library"
#define ACNETWORK_LIBRARY_VERSION 1

#define ACNETWORK_STATE_CARD   0x01
#define ACNETWORK_STATE_ONLINE 0x02

#define ACNETWORK_CMD_NEIGHBOURS 0x35
#define ACNETWORK_NEIGH_VALID    0x01

struct ACNetworkNeighbour {
    UBYTE ipv4[4];
    UBYTE mac[6];
    UBYTE state;
    UBYTE reserved;
    ULONG ifindex;
    char ifname[16];
};
typedef char ACNetworkNeighbour_size_must_be_32[(sizeof(struct ACNetworkNeighbour) == 32) ? 1 : -1];

struct ACNetworkRequest {
    ULONG command;
    ULONG arg[4];
    const UBYTE *tx;
    ULONG txlen;
    UBYTE *rx;
    ULONG rxmax;
    LONG result;
    ULONG error;
    ULONG rxlen;
};

#define ACNETWORK_LVO_CALL       (-30)
#define ACNETWORK_LVO_ARM        (-36)
#define ACNETWORK_LVO_DISARM     (-42)
#define ACNETWORK_LVO_SIGNALMASK (-48)
#define ACNETWORK_LVO_LASTERROR  (-54)
#define ACNETWORK_LVO_STATE      (-60)

#endif
