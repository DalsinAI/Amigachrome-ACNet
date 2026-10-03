# ACNet Status

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

1. Merge/deploy the matching guest and host drop-in branches as one protocol set.
2. Re-run the consolidated Instance-6 qualification after deployment.
3. Exercise real application compatibility across FTP, browser, IRC, SimpleMail, long transfers and other Roadshow/AmiTCP-shaped software.
4. Run the broader bsdsock conformance suite and document deliberate limits.
5. Finish Cradle packaging/conflict handling for a genuine one-switch drop-in.
6. Defer Wi-Fi, event log, BPF/libpcap, mutable admin, SANA-II and IPv6 until the drop-in gate is secure.
