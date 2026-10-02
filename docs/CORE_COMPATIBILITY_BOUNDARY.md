# ACNet Core And Compatibility Boundary

ACNet is an independent Amiga `bsdsocket.library` implementation. Its architecture is not derived from, and must not depend on, the internals or administration model of any other TCP/IP stack.

## Default core

The default build contains:

- the classic Amiga BSD socket application ABI;
- per-opener library bases and descriptor state;
- TCP/UDP socket calls through the provider boundary;
- `WaitSelect`, signals, errno/h_errno and descriptor handoff;
- DNS/netdb calls needed by ordinary applications;
- `sendmsg`/`recvmsg`;
- the original generic `SocketBaseTagList` opener controls;
- ten reserved ABI slots for compatible growth.

The vector table is generated from `guest/network/acnet/abi/bsdsocket-v4.json`, an ACNet-owned manifest. The default build does not read a third-party socket-library SFD.

## Not core

Routing administration, interface administration, monitoring hooks, BPF, mbuf/kernel-memory access, packet filtering, global-stack-data access and stack-specific feature-probe tags are not part of ACNet core.

If real application evidence requires one of these interfaces, implement it under `guest/network/acnet/compat/` as an explicit compatibility profile. Such code must translate at the boundary into ACNet-neutral operations; ACNet internals must never become shaped around that external ABI.

`usergroup.library` is likewise an optional legacy-runtime compatibility component, not an ACNet networking dependency.

## Configuration

IP addressing, DNS, activation, host/Wi-Fi policy, statistics and logs belong to the ACNet control plane (`acnet.device`/HostSocket/ACNetControl), not to another stack's administration API.

## Physical vector safety envelope

The logical core ABI ends at slot 56: 46 classic application vectors followed by
10 reserved slots. The physical library table is deliberately longer. Slots 57-139
are typed failure-only guards matching the extent of a widely deployed larger legacy
ABI. This is a crash-safety measure, not API compatibility and not an implementation
dependency.

ACNet never assigns new meanings to slots 47-139. Any future ACNet library extension
starts at slot 140 or later. Prefer `acnet.device`/HostSocket/ACNetControl for new
configuration, status and lifecycle features so `bsdsocket.library` remains small and
stable.
