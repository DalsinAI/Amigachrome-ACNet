#ifndef ACNET_PROVIDER_H
#define ACNET_PROVIDER_H

/* What ACNet's bsdsocket.library front end asks of a socket provider: the
 * HostSocket command set (common/protocol/achostsocket.h) as calls, and a
 * wake-up when the provider has news. HostSocket over acnet.device is the
 * first provider (provider_hostsocket.c). An lwIP provider over SANA-II, for
 * real Amigas, would answer the same commands in-process. */
#include "acnet_lib.h"

/* Per opener: 0, or the errno OpenLibrary fails with. */
LONG prov_open(struct SocketBase *sb);
void prov_close(struct SocketBase *sb);
/* One command: its result (-1 on failure, with sb->last_err set). Up to
 * rxmax bytes of the answer land in rx; *rxlen says how many the provider had. */
LONG prov_call(struct SocketBase *sb, ULONG cmd, ULONG a0, ULONG a1, ULONG a2, ULONG a3,
               const void *tx, ULONG txlen, void *rx, ULONG rxmax, ULONG *rxlen);
/* From prov_arm until prov_disarm, the provider's next event signals this
 * task with sb->waiter.sigmask. Arm before polling, so no event is missed. */
void prov_arm(struct SocketBase *sb);
void prov_disarm(struct SocketBase *sb);

#endif
