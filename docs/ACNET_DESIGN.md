# ACNet: networking for AmigaOS 3.2.3 on AmigaChrome

**Status:** Drop-in IPv4 compatibility implemented and qualifying · **Date:** 3 October 2026 · **Guest branch:** `dropin/compat-control-20261003` · **Host branch:** `net/dropin-state-20261003`
**Created by** Dale Kirkwood, in collaboration with Thufir Hawat.

This is the build design. It takes the 30 September designs (capsules ACNet BSDSocket CX Design and ACNet FastPath HostSocket ACNetDev SANA2) and fixes the shape Dale set on 1 October.

## 1. What we are building

| Part | What it is |
| --- | --- |
| `LIBS:bsdsocket.library` | Classic Amiga BSD-socket compatibility facade. The core v4 ABI now has all 46 application vectors implemented; optional Roadshow-shaped compatibility lives behind the reserved guard tail. |
| `LIBS:acnetwork.library` | Native AmigaChrome networking boundary below `bsdsocket.library`. Native AC software may call it directly for status, interfaces, routes, neighbours, DNS, sockets and control without inheriting legacy BSD policy. |
| ACNet, the card | A Zorro II card in the autoconfig chain: Dalsin, product 6, 64 KB. ShowConfig and SysInfo list it. It is fitted while the instance's Network switch is on. |
| `DEVS:acnet.device` | The one driver that touches the card's HostSocket block. It owns the lock and the interrupt and wakes waiting tasks. It gives the library private direct calls, with no I/O request per packet. |
| `DEVS:acwifi.device` | Control of the host's Wi-Fi through the same card: scan, status, join, leave, forget. |
| ACNetControl | ReAction Commodity with Status, Wi-Fi, Connections, Diagnostics and Log pages. Live status/connection views, Refresh, soft online/offline, DNS lookup, TCP connect test and support-report save are implemented. |
| ACTCPTools | Native OS 3.2.3 command suite: `hostname`, `resolve`, `ping`, `traceroute`, `arp`, `ifconfig`, `route`, `netstat`, `acnetctl`. |
| The host service | Part of the instance's bridge on Linux. It owns the real sockets, DNS and the Wi-Fi requests. |

```
 classic Amiga app               AmigaChrome-native app / ACNetControl
        |                                     |
 bsdsocket.library                       acnetwork.library
        |                                     ^
        +------------> acnetwork.library -----+
                              |
                         acnet.device
                              |
                 ACNet Zorro II card / HostSocket
                              |
                    instance bridge on Linux
                    sockets, DNS, state/control

 acwifi.device remains the future Wi-Fi control path on the same card.
```

Linux does the TCP/IP. The Amiga side is a thin layer that tests can pin down. That is what makes it fast and reliable on a 68k. It is the direction the PiStorm and Emu68 networking work took: a purpose-built path, not SANA-II. WinUAE's bsdsocket emulation is the precedent for passing sockets through.

## 2. Decisions (Dale, 1 October 2026)

1. **The library first.** OS 3.2.3 programs reach the network through ACNet's `bsdsocket.library`. A SANA-II card is not on its path.
2. **Off until switched on.** Each instance has a Network switch in Cradle's hardware panel. The library still opens with the switch off, and reports the network down (ENETDOWN).
3. **Internet and LAN, not this PC.** 127.0.0.1 is the Amiga's own loopback. The PC's own addresses are refused, which keeps out Cradle, other instances' bridges and other services on the PC.
4. **Two drivers, one card.** `acnet.device` and `acwifi.device` both talk to ACNet, which shows in autoconfig as a card in its own right (Dalsin product 6, the number main reserved for networking). Dale added this on 1 October. Its top half is kept for the later packet rings. Product 8 stays reserved.
5. **One Commodity**, ACNetControl, written in ReAction, with a Wi-Fi tab. ReAction ships with OS 3.2.3 and NDK 3.2 has its headers; MUI would be a third-party install. This merges the 30 September design's two Commodities.
6. **Host Wi-Fi control is a separate per-instance permission, off by default.** Joining a network changes the whole PC's Wi-Fi.
7. **No paid licence and no weak spots.** The library must pass bsdsocktest and a matrix of real programs before release.

## 3. bsdsocket.library

- **Interface.** ACNet owns a checked-in classic Amiga BSD socket ABI manifest: 46 application vectors followed by 10 reserved growth slots. The vector table is generated from that manifest, not from a third-party stack SFD. As of 3 October all 46 core application vectors are implemented; the core has no remaining `ENOSYS` stubs.
- **Per opener.** Every `OpenLibrary()` returns a base of its own, following the classic Amiga socket model. Each base holds its own descriptors, errno, signals, h_errno and name buffers. Closing it closes its sockets.
- **Waiting.**
  - Blocking calls, WaitSelect and name lookups sleep on Exec signals. The device's interrupt wakes them; nothing polls.
  - Ctrl-C (or the opener's break mask) interrupts a wait with EINTR.
  - WaitSelect takes the caller's signal mask and a timeout (timer.device).
- **Name and network database lookups** run on the host, asynchronously where required, so a slow resolver never freezes the machine. `gethostbyname`, `gethostbyaddr`, `getservbyname`, `getservbyport`, `getnetbyname` and `getnetbyaddr` are implemented. Protocols come from a small built-in table; `gethostid()` is derived from the guest-facing ACNet IPv4 interface.
- **Moving sockets between tasks.** ObtainSocket, ReleaseSocket, ReleaseCopyOfSocket and Dup2Socket work. Servers started inetd-style depend on them.
- **Interfaces and routes.** Native state comes from `acnetwork.library`. `bsdsocket.library` contains an optional Roadshow-shaped compatibility adapter for read-only interface, route/ARP, DNS and statistics queries. Mutable Roadshow administration remains guarded rather than pretending Linux-owned state lives inside an Amiga TCP/IP stack.
- **Version:** 4.x, preserving the classic Amiga `bsdsocket.library` application ABI. The id string says ACNet.

## 4. acnet.device

- It finds the ACNet card (Dalsin $DA15, or 2011 for older ROMs, product 6) through expansion.library. It checks the HostSocket block's signature ("ACH1") and version, then adds one INT2 server. With no card it opens anyway and reports the network absent.
- **Private calls** (vectors after the standard device ones):
  - `ACN_Call`: one command. It writes the arguments, copies the transmit buffer, starts the command and copies the answer, all under Forbid().
  - `ACN_AddWaiter` and `ACN_RemWaiter`: a task and its signal, woken on each event.
  - `ACN_State`: present, and online.
- The library opens it once per opener and calls those vectors directly.
- **Later:** a packet-oriented compatibility path may be added for tools that genuinely need one. It remains independent of the application socket path.

## 5. acwifi.device

- The same card and the same discipline as acnet.device, using command range $40-$4F.
- **Calls:** Scan, Networks (SSID, signal, security, known), Status (SSID, BSSID, signal, channel, security, link rate), Join (a known network), Leave, Forget. A scan returns a ticket and the answer follows with an event.
- **Host side:** the bridge asks NetworkManager (`nmcli`). Passphrases never enter or stay in the Amiga.
- **Permission:** refused (EPERM) unless the instance's Wi-Fi control permission is on.

## 6. ACNetControl

- A real `commodities.library` broker using stock OS 3.2.3 ReAction. Exchange Show/Hide and the Ctrl-Alt-N hotkey expose the window; Kill quits only the control app.
- **Status:** live card/online state, hostname, IPv4, mask, gateway, DNS, host link, interface, open-socket count, byte counters, connects and refusals.
- **Connections:** read-only live socket inventory for the instance.
- **Controls:** Refresh and soft Go online/Go offline call the native ACNetwork control path.
- **Diagnostics:** editable DNS lookup and TCP host:port connection tests go through the public `bsdsocket.library` ABI; support snapshots save to `RAM:ACNetReport.txt`.
- **Wi-Fi and Log:** no fake buttons. Until `acwifi.device` and the event-ring backend are fitted, those pages say so explicitly.
- The stock-ReAction autoinit bug was fixed by strongly defining the class bases; the current build is warning-free.
- Instance-23 carries the current test copy under `SYS:Tools/ACNetwork Tools/ACNetControl`.

## 7. The host service

- It runs in the instance bridge (`hostsocket.py`, beside `native_bridge.py`). Host sockets are non-blocking. The service never blocks the runtime longer than one non-blocking call.
- **Handles** are small integers per instance. Host descriptors are never exposed.
- **Events:** the guest's POLL names the sockets it waits on. A watcher thread posts one event, input record 11, when any of them, or a pending DNS answer, becomes ready. The runtime holds INT2 while the event is pending and IRQ_ENABLE is set.
- **Policy:**
  - Off unless the instance's switch is on; it is read again on every HELLO and SOCKET.
  - TCP and UDP are IPv4-only in v1. General raw sockets remain refused. Two constrained compatibility personalities are implemented: classic ICMP Echo is virtualised over Linux unprivileged datagram-ICMP sockets, and traceroute raw-UDP/IP probes are translated through Linux UDP plus the error queue. Other raw protocols remain refused.
  - Connects and datagrams to 127.0.0.0/8, 0.0.0.0/8, multicast or the PC's own addresses are refused (ECONNREFUSED or ENETUNREACH).
  - Guest loopback works: a connect to 127.0.0.1:P reaches the same instance's own listener on P.
- **Errno, levels and options** use the Amiga's (BSD) numbers on the board and are mapped to Linux's in the service.

## 8. The board

ACNet is a 64 KB Zorro II card: Dalsin $DA15 (or 2011, as ACBridge), product 6, serial 1. It comes after ACStorage in the chain. It is fitted when the runtime starts with `-n`, and the bridge fits or pulls it from the next reset with input record 12, as the Network switch changes. Turning the switch off takes the network down at once; the card goes at the next reboot.

HostSocket's block starts at the card's base:
- registers at $0000 to $00FF;
- a 16,128-byte transmit buffer at $0100;
- a 16,384-byte receive buffer at $4000;
- $8000 to $FFFF kept for the packet rings of the later SANA-II side.

The registers and commands are in `vendor/amigachrome-guest/common/protocol/achostsocket.h`. A command is synchronous: writing COMMAND runs it, and RESULT, ERRNO and RXLEN are set when the write returns. EVENT plus IRQ_ENABLE gives INT2. The runtime and the bridge talk over the existing request pipe as board 3, and over the input pipe with record 11.

## 9. Cradle

- The hardware panel gains a **Network** switch and a **Host Wi-Fi control** switch. Both are off.
- Switching Network on fits the card from the next reboot and installs `LIBS:bsdsocket.library`, `LIBS:acnetwork.library`, `DEVS:acnet.device`, ACNetControl and the native ACNetwork Tools command suite. `acwifi.device` is installed only when that backend is actually fitted. Existing third-party `bsdsocket.library` installations are detected as conflicts rather than silently overwritten.

## 10. Quality bar

- **Host:** focused HostSocket qualification currently runs 29 tests successfully with 2 expected skips, covering policy, handles, POLL/events, DNS, ICMP/traceroute translation, state/control views and network-database lookups.
- **Board:** board_test cases for the block's registers, buffers and INT2.
- **Guest, under genuine AmigaOS 3.2.3:** core BSD qualification, ACTCPTools first-light and the Roadshow-shaped read-only probe are live-qualified on Instance-6. `ping`, `traceroute`, `arp`, `ifconfig`, `route`, `netstat`, `hostname`, `resolve` and `acnetctl` have working OS3 builds; the Roadshow read-only probe completed with 0 failures.
- **Conformance:** bsdsocktest (tbdye, 142 tests). The target is everything attempted, no unexpected failures, and every limit written down.
- **Programs:** an FTP client, a browser, an IRC client, SimpleMail, and a long download.
- **Never:** busy polling, a wait that can't be broken, or a library open that can hang.

## 11. Limits in version 1

- IPv4 only. IPv6 is a separate future feature rather than a compatibility patch.
- General raw sockets remain unavailable; only the constrained ICMP Echo and traceroute personalities are virtualised.
- Programs that drive SANA-II directly still need a packet-oriented/SANA-II side of `acnet.device`.
- Mutable Roadshow administration (route/interface/DNS mutation) remains guarded; the current compatibility layer is deliberately read-only.
- `acwifi.device`, host Wi-Fi scan/join/leave and the ACNet event-ring/log backend are not yet fitted.
- BPF/libpcap/tcpdump compatibility has not yet been implemented.
- Instance-23 currently has the newer tools/CX binary, but its installed HostSocket/runtime remains older until the host branch is merged and deployed.
- AROS 68k keeps its own stack. ACNet is OS 3.2.3 first.

## 12. Relationship to earlier designs

- **Kept:** the 30 September ACNet designs' interface, their test gates and their security boundary. Their two Commodities are merged into one.
- **Superseded:**
  - the 27 September ACNet v0.1 UDP-only stack;
  - the statement in the 27 September Network/Wi-Fi design that AmigaChrome would not supply bsdsocket.library;
  - that design's bridge-by-default rule (now off by default).
- **Still valid:** the ACNetwork (6) and ACWifiNet (8) board work, for the later packet-level card.

## 13. Remaining work to drop-in release

1. Merge the qualified guest branch `dropin/compat-control-20261003` and host branch `net/dropin-state-20261003` to their release integration points.
2. Deploy only through the normal AmigaChrome deployment path, then restart a dedicated qualification guest and confirm the host/runtime protocol versions match.
3. Re-run the consolidated OS 3.2.3 gate: core BSD qualification, HostSocket focused tests, ACTCPTools, Roadshow read-only probe and ACNetControl live telemetry/controls.
4. Run the real-application compatibility matrix: FTP, browser, IRC, SimpleMail, long transfer and representative third-party software that expects Roadshow/AmiTCP-shaped behaviour.
5. Run the broader bsdsock conformance suite and document every deliberate limitation.
6. Finish packaging/install conflict handling so ACNet is a genuine one-switch drop-in without overwriting third-party networking.
7. Only after the drop-in gate: decide priorities for Wi-Fi control, event logging, BPF/libpcap/tcpdump, mutable admin APIs, SANA-II and IPv6.

## 14. Open decisions / later lanes

- Whether Wi-Fi may join networks the PC does not already know; version 1 should continue to avoid carrying passphrases through the guest unless explicitly designed.
- When packet/SANA-II compatibility earns its cost from real software evidence.
- Whether mutable route/interface/DNS administration should ever be exposed from the guest, rather than remaining host-owned.
- Whether BPF/libpcap/tcpdump belongs in the first drop-in release or the following compatibility release.
- IPv6 scope and guest ABI remain deliberately undefined.
