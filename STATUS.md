# ACNet Status

> **Renamed 4 October 2026.** ACNet is now OpenSocket: `acnet.device` is `DEVS:Networks/opensocket.device`, `acnetwork.library` is `opensocket.library`, `acwifi.device` is `opensocketwifi.device`, ACNetControlGT is OpenSocketControl (the ReAction version is retired), `acnetctl` is `C:OpenSocket`, the tools are in `SYS:Tools/OpenSocket/` and the interface is `opensocket0`. The current design is `docs/architecture/OPENSOCKET_DESIGN.md` in DalsinAI/amigachrome. The entries below keep the names of their time.

## 2026-10-03 - Complete-stack code milestone

The specified AmigaOS 3.2.3 ACNet stack is now implemented in code on the complete-stack branches. The remaining release work is matching guest/host integration and live qualification, not invention of another TCP/IP core.

New code since the drop-in checkpoint:

- modern IPv4 compatibility APIs: re-entrant host lookups, addrinfo/nameinfo and inet conversion helpers;
- bounded native event log and ACNetControl Log page;
- `acwifi.device` plus asynchronous, permission-gated NetworkManager scan/status/known-profile control;
- SANA-II packet facade on `acnet.device` using an unprivileged libslirp Ethernet provider;
- Roadshow-compatible BPF channels, classic-BPF VM/filtering, capture, injection, timeouts and notification masks;
- proper BSD errno/h_errno text;
- hash-verified transactional Cradle guest installer with foreign-`bsdsocket.library` refusal and ACFS metadata updates;
- self-contained 14-file production payload plus separate qualification probes.

Build/test evidence:

- 46/46 classic core vectors, zero core stubs;
- 27 implemented Roadshow compatibility-tail vectors;
- complete OS3 guest stack and ACTCPTools cross-build;
- HostSocket complete-stack suite: 35/35;
- installer + hardware + HostSocket combined gate: 56/56;
- transactional installer suite: 6/6;
- real Roadshow AmigaOS libpcap 0.8.1 source cross-built to a 140 KiB m68k archive against ACNet BPF.

Packet-level IPv6 is available through the libslirp/SANA-II path. The classic OS3/Roadshow socket ABI remains IPv4 because the target NDK contains no public AF_INET6/sockaddr_in6 ABI.

See `docs/architecture/ACNET_STACK_AND_API_REFERENCE.md` for the canonical component/API reference.

## 2026-10-03 - Drop-in IPv4 compatibility

ACNet now has a public native control plane (`acnetwork.library`) plus a read-only
Roadshow-shaped compatibility adapter in `bsdsocket.library`. The adapter exposes
guest-facing `acnet0` state while mutable host/network administration remains guarded.

Current qualified capabilities:

- core BSD sockets: TCP, UDP, DNS, WaitSelect/non-blocking and constrained ICMP;
- ACTCPTools: hostname, resolve, ping, traceroute, arp, ifconfig, route, netstat, acnetctl;
- native ACNetwork views: status, interfaces, routes, neighbours, DNS and this-instance sockets;
- ACNetControl: live status/connections, Refresh, soft online/offline, public-BSD DNS/TCP diagnostics and support report;
- Roadshow read-only APIs: routes/ARP, interfaces, DNS list and network/socket statistics;
- `gethostid()`, `getnetbyname()` and `getnetbyaddr()` implemented;
- logical core: all 46 application vectors implemented, 0 core stubs;
- compatibility guard tail: 83 slots, 8 read-only implementations; all other slots remain guards.

Qualification:

- HostSocket focused suite: 29 tests run, OK, skipped=2;
- Roadshow-shaped OS 3.2.3 probe: PASS, 0 failures, RC 0;
- live interface `acnet0`: IPv4, MTU 1500, 1 Gbit/s link, Ethernet identity;
- normal route enumeration: 2 live routes;
- DNS enumeration: live resolver server returned;
- TCP/UDP socket/status APIs: PASS;
- Instance-6 restored byte-for-byte after live qualification.
## 2026-10-02 - Clean Core

ACNet's default `bsdsocket.library` is now independent of Roadshow-specific ABI material. The vector table is generated from ACNet's own classic Amiga BSD socket manifest.

Current build outputs:

- `acnet.device`: 3,628 bytes
- `bsdsocket.library`: 20,088 bytes
- `acnettest`: 5,208 bytes
- `bsdqual`: 16,928 bytes
- `ACNetControl`: 11,932 bytes
- logical core ABI: 46 classic application vectors + 10 permanently reserved slots
- physical vector table: 139 slots (56 core/reserved + 83 compatibility guards)
- implemented application vectors: 43
- honest core stubs: 3 (`getnetbyname`, `getnetbyaddr`, `gethostid`)

Qualification:

- clean-core guest qualification: 47/47 pass, RC 0
- tiny DNS/TCP guest smoke: PASS, RC 0
- compatibility guard probe: 3/3 pass (slot 57 scalar, slot 69 pointer, slot 139 end-of-table)
- HostSocket host tests: 21/21 pass
- ACNet-enabled native A1200 board tests: 89/89 pass
- C-Dogs SDL fresh rebuild: RC 0
- OpenOMF fresh rebuild: RC 0

Core boundary:

- no third-party socket-library SFD is read by the default build
- stack-specific routing/interface/monitoring/BPF/mbuf/administrative APIs are outside core
- optional legacy compatibility belongs under `guest/network/acnet/compat/`
- slots 47-139 are never reused by ACNet; future library extensions begin at slot 140 or later
- `usergroup.library` is not an ACNet networking dependency

## 2026-10-01 - First Light

Initial AmigaOS 3.2.3 guest proof:

- library open: PASS
- hostname: PASS (`instance-6`)
- DNS localhost: PASS
- TCP loopback bind/listen/connect/accept: PASS
- send/recv payload verification: PASS
- guest smoke program return code: 0

The original first-light build used a broader compatibility-shaped vector table. The 2 October clean-core milestone supersedes that as the default architecture.

## Next qualification targets

1. Commit/merge and deploy the matching complete-stack guest and host/runtime branches as one protocol set.
2. Run the consolidated Instance-6 campaign, including modern compatibility, BPF, SANA-II, Wi-Fi, event-log and installer paths as well as the existing core/tool probes.
3. Qualify the Cradle one-switch installer on a disposable OS3 volume before normal release deployment.
4. Exercise real application compatibility across FTP, browser, IRC, SimpleMail, long transfers and other Roadshow/AmiTCP-shaped software.
5. Run the broader bsdsock conformance suite and document deliberate limits.
6. Decide later-policy items separately: mutable host administration, a native ACNet IPv6 socket ABI, high-throughput packet rings and tcpdump source-package repair.
