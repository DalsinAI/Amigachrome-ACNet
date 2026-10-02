#ifndef ACNETWORK_H
#define ACNETWORK_H

#include <exec/types.h>

#define ACNETWORK_LIBRARY_NAME "acnetwork.library"
#define ACNETWORK_LIBRARY_VERSION 1

#define ACNETWORK_STATE_CARD   0x01
#define ACNETWORK_STATE_ONLINE 0x02

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
