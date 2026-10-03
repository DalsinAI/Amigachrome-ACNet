# Optional Roadshow compatibility

ACNet's `bsdsocket.library` keeps its classic-Amiga BSD socket core independent
of Roadshow-specific internals. A deliberately limited compatibility profile in
this directory translates useful read-only Roadshow administration/status calls
onto the public `acnetwork.library` control plane.

Roadshow compatibility must remain an adapter. It must not become an internal
dependency of the ACNet socket core, HostSocket provider, `acnet.device`, or
ACNetControl. Mutable stack administration, BPF, mbuf, kernel-memory, server,
global-data and IP-filter APIs remain outside the core unless real application
evidence justifies a compatibility implementation.

`usergroup.library` is likewise not part of the ACNet core.  If a particular
legacy runtime requires it, treat it as an optional application-compatibility
package with its own tests and release boundary.

## Guard tail

The default ACNet binary carries a failure-only compatibility guard from physical
vector slot 57 through slot 139. These entries are not ACNet APIs. They exist so a
program compiled for a larger legacy `bsdsocket.library` ABI receives a clean
`ENOSYS`/NULL/FALSE result instead of jumping beyond the library.

Slots 47-56 remain permanently reserved. ACNet will not reuse them. Slots 57-139
are permanently reserved as the compatibility safety envelope. If ACNet ever adds
new library vectors, they begin at slot 140 or later; new control/status work should
prefer the ACNet control plane instead.

The read-only profile currently implements eight slots inside that envelope:

- `FreeRouteInfo` / `GetRouteInfo` (68/69): ARP `RTF_LLINFO` and normal IPv4 routes;
- `ReleaseInterfaceList` / `ObtainInterfaceList` / `QueryInterfaceTagList` (72-74);
- `GetNetworkStatistics` (81), including TCP/UDP socket inventories;
- `ReleaseDomainNameServerList` / `ObtainDomainNameServerList` (84/85).

Capability tags advertise only the read-only routing, interface, status and DNS
APIs that are present. The compatibility view exposes a guest-facing `acnet0`
interface backed by `acnet.device`; host interface names do not leak through this
ABI. Route/interface/DNS mutators and every other unimplemented extension remain
failure guards.

`tests/guard_probe.c` checks the guard envelope. `tests/roadshow_readonly_probe.c`
opens `bsdsocket.library` normally and qualifies the supported Roadshow-shaped API
without calling any ACNet-private interface.
