# ACNet drop-in compatibility current position

> **Historical checkpoint — superseded later on 3 October 2026 by the complete-stack milestone.** For current architecture/API state read `docs/architecture/ACNET_STACK_AND_API_REFERENCE.md` and `docs/ACNET_DESIGN.md`.

**Date:** 3 October 2026
**Purpose:** restart capsule for the ACNet IPv4 drop-in compatibility lane.

## Active branches

- Guest/network repo: `dropin/compat-control-20261003`.
- Guest head after current work: `4e7f7ad` — ACNetControl working DNS/TCP diagnostics.
- Immediately below: `99b761a` — complete BSD network-database lookups.
- Host/runtime repo: `net/dropin-state-20261003`.
- Host head: `8718408` — complete network-database lookup support.

Do not assume either branch is merged to main. Fetch and recheck before integration.

## Architecture

```text
classic Amiga app                 native AC software / ACNetControl
        |                                      |
 bsdsocket.library                        acnetwork.library
        |                                      ^
        +-------------> acnetwork.library -----+
                               |
                          acnet.device
                               |
                  ACNet Zorro II / HostSocket
                               |
                         Linux networking
```

## What is implemented

- All 46 classic BSD application vectors are implemented; no core `ENOSYS` stubs remain.
- TCP, UDP, DNS, WaitSelect/non-blocking and guest loopback are working.
- Constrained ICMP Echo compatibility works without host raw-socket privilege.
- Traceroute guest raw-IP/UDP probes are translated to Linux UDP/error-queue ICMP.
- Native ACNetwork state/control views cover status, interfaces, routes, neighbours, DNS, sockets and soft online/offline.
- Read-only Roadshow-shaped compatibility covers routes/ARP, interfaces, DNS and network/socket statistics.
- `gethostid`, `getnetbyname` and `getnetbyaddr` are implemented.

ACTCPTools currently builds:

- `hostname`
- `resolve`
- `ping`
- `traceroute`
- `arp`
- `ifconfig`
- `route`
- `netstat`
- `acnetctl`

## ACNetControl

The current ReAction Commodity is no longer the first-light mock-up.

- live Status and Connections views consume `acnetwork.library`;
- Refresh is real;
- Go online / Go offline is real soft control beneath the Cradle Network switch;
- DNS Lookup is a real public `bsdsocket.library` call;
- Test TCP is a real public `bsdsocket.library` connect path;
- Save report writes `RAM:ACNetReport.txt`;
- Wi-Fi and Log pages deliberately show that their backends are not fitted instead of displaying dead buttons;
- stock OS 3.2.3 ReAction autoinit was fixed by strongly defining the class bases.

Instance-23 currently contains the current CX and command binaries in:

`SYS:Tools/ACNetwork Tools/`

Important: Instance-23's installed HostSocket/runtime is still older than the drop-in host branch. The new tools/CX are present, but full live state/control must not be judged until the matching host/runtime is merged and deployed.

## Qualification evidence

- HostSocket focused suite: 29 tests pass, 2 expected skips.
- Roadshow-shaped read-only OS 3.2.3 probe: 0 failures.
- `ping`, `traceroute`, `arp`, `ifconfig`, `route`, `netstat`, `hostname`, `resolve` and `acnetctl` have live-qualified paths from the campaign.
- Instance-6 is the dedicated ACNet qualification guest; restore its startup/files after each campaign.

## What still needs to be done

### Required for the first drop-in release

1. Fetch/recheck both active branches and merge the guest and host work through the normal review path.
2. Deploy the matching HostSocket/runtime and guest libraries as one protocol set; do not mix old host with new guest.
3. Run the consolidated Instance-6 gate after deployment: core BSD qualification, HostSocket suite, ACTCPTools, Roadshow read-only probe and ACNetControl telemetry/actions.
4. Run the wider real-application matrix: FTP, browser, IRC, SimpleMail, long transfer and representative third-party Roadshow/AmiTCP software.
5. Run the broader bsdsock conformance suite and record deliberate incompatibilities.
6. Finish Cradle packaging/install conflict handling so enabling ACNet never silently overwrites an existing third-party `bsdsocket.library`.

### Explicit later work, not blockers for basic IPv4 sockets

- `acwifi.device` plus scan/join/leave and host NetworkManager control;
- ACNet event-ring/log backend;
- BPF/libpcap/tcpdump compatibility;
- mutable Roadshow route/interface/DNS administration, only if justified;
- packet/SANA-II compatibility for software that bypasses `bsdsocket.library`;
- IPv6.

## Restart rule

Start by reading `docs/ACNET_DESIGN.md` and this capsule. Treat the native `acnetwork.library` API as authoritative; classic/Roadshow compatibility translates onto it. Do not reintroduce a second Amiga TCP/IP engine and do not grant unrestricted host raw sockets.
