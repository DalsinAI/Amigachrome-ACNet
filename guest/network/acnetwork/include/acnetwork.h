#ifndef ACNETWORK_H
#define ACNETWORK_H

#include <exec/types.h>

#define ACNETWORK_LIBRARY_NAME "acnetwork.library"
#define ACNETWORK_LIBRARY_VERSION 1

#define ACNETWORK_STATE_CARD   0x01
#define ACNETWORK_STATE_ONLINE 0x02

#define ACNETWORK_CMD_STATUS       0x02
#define ACNETWORK_CMD_HOSTNAME     0x34
#define ACNETWORK_CMD_NEIGHBOURS   0x35
#define ACNETWORK_CMD_INTERFACES   0x36
#define ACNETWORK_CMD_ROUTES       0x37
#define ACNETWORK_CMD_SOCKETS      0x38
#define ACNETWORK_CMD_SET_ONLINE   0x39
#define ACNETWORK_CMD_DNS_SERVERS  0x3a
#define ACNETWORK_CMD_LOG          0x3d

#define ACNETWORK_NEIGH_VALID      0x01
#define ACNETWORK_SOCKET_OPEN      0
#define ACNETWORK_SOCKET_BOUND     1
#define ACNETWORK_SOCKET_LISTEN    2
#define ACNETWORK_SOCKET_CONNECTED 3

struct ACNetworkStatus {
    ULONG state;
    ULONG link_mbps;
    ULONG sockets;
    ULONG reserved;
    unsigned long long bytes_in;
    unsigned long long bytes_out;
    ULONG connects;
    ULONG refused;
};
typedef char ACNetworkStatus_size_must_be_40[(sizeof(struct ACNetworkStatus) == 40) ? 1 : -1];

struct ACNetworkNeighbour {
    UBYTE ipv4[4];
    UBYTE mac[6];
    UBYTE state;
    UBYTE reserved;
    ULONG ifindex;
    char ifname[16];
};
typedef char ACNetworkNeighbour_size_must_be_32[(sizeof(struct ACNetworkNeighbour) == 32) ? 1 : -1];

struct ACNetworkInterface {
    ULONG ifindex;
    ULONG flags;
    UBYTE ipv4[4];
    UBYTE netmask[4];
    UBYTE mac[6];
    UBYTE reserved0[2];
    ULONG mtu;
    ULONG link_mbps;
    unsigned long long rx_bytes;
    unsigned long long tx_bytes;
    char ifname[16];
};
typedef char ACNetworkInterface_size_must_be_64[(sizeof(struct ACNetworkInterface) == 64) ? 1 : -1];

struct ACNetworkRoute {
    UBYTE destination[4];
    UBYTE gateway[4];
    UBYTE netmask[4];
    ULONG ifindex;
    ULONG flags;
    ULONG metric;
    char ifname[16];
};
typedef char ACNetworkRoute_size_must_be_40[(sizeof(struct ACNetworkRoute) == 40) ? 1 : -1];

struct ACNetworkSocket {
    ULONG handle;
    ULONG kind;
    ULONG protocol;
    ULONG state;
    UBYTE local_ipv4[4];
    UWORD local_port;
    UBYTE reserved0[2];
    UBYTE remote_ipv4[4];
    UWORD remote_port;
    UBYTE reserved1[2];
    char label[16];
};
typedef char ACNetworkSocket_size_must_be_48[(sizeof(struct ACNetworkSocket) == 48) ? 1 : -1];

#define ACNETWORK_LOG_INFO  1
#define ACNETWORK_LOG_WARN  2
#define ACNETWORK_LOG_ERROR 3

struct ACNetworkLogEntry {
    ULONG sequence;
    ULONG seconds;
    ULONG level;
    char text[84];
};
typedef char ACNetworkLogEntry_size_must_be_96[(sizeof(struct ACNetworkLogEntry) == 96) ? 1 : -1];

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
