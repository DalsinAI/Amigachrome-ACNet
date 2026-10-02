# Optional Roadshow compatibility

ACNet's default `bsdsocket.library` is intentionally a clean classic-Amiga BSD
socket implementation.  It does not model Roadshow's routing, interface,
monitoring, BPF, mbuf, kernel-memory, server, global-data or IP-filter APIs.

If application evidence later justifies one of those extensions it belongs in
this directory as an explicit compatibility module/build profile.  It must not
become an internal dependency of the ACNet socket core, HostSocket provider,
`acnet.device`, or ACNetControl.

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

Application evidence from ACTCPTools `arp` now enables two slots inside that envelope:
`FreeRouteInfo` (68) and `GetRouteInfo` (69). The implementation is deliberately
read-only and accepts only the IPv4 `RTF_LLINFO` neighbour-table query. All route
mutation and all other Roadshow extension slots remain guarded/failure-only.

`tests/guard_probe.c` raw-calls slot 57, a pointer-returning guard slot, and slot 139
inside AmigaOS to verify the envelope fails safely.
