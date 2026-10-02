# ACNet: networking for AmigaOS 3.2.3 on AmigaChrome

**Status:** Specified, building · **Date:** 1 October 2026 · **Branch:** `net/hostsocket-20261001`
**Created by** Dale Kirkwood, in collaboration with Thufir Hawat.

This is the build design. It takes the 30 September designs (capsules ACNet BSDSocket CX Design and ACNet FastPath HostSocket ACNetDev SANA2) and fixes the shape Dale set on 1 October.

## 1. What we are building

| Part | What it is |
| --- | --- |
| `LIBS:bsdsocket.library` | ACNet. Our own BSD-3 library implementing the classic Amiga BSD socket application interface over HostSocket. It has no TCP stack of its own and does not require SANA-II. |
| ACNet, the card | A Zorro II card in the autoconfig chain: Dalsin, product 6, 64 KB. ShowConfig and SysInfo list it. It is fitted while the instance's Network switch is on. |
| `DEVS:acnet.device` | The one driver that touches the card's HostSocket block. It owns the lock and the interrupt and wakes waiting tasks. It gives the library private direct calls, with no I/O request per packet. |
| `DEVS:acwifi.device` | Control of the host's Wi-Fi through the same card: scan, status, join, leave, forget. |
| ACNetControl | A ReAction Commodity with Status, Wi-Fi and Diagnostics tabs. |
| The host service | Part of the instance's bridge on Linux. It owns the real sockets, DNS and the Wi-Fi requests. |

```
 Amiga program
      |  bsdsocket.library (ACNet)        ACNetControl (ReAction CX)
      |          |                          |            |
      |    acnet.device  <------------------+       acwifi.device
      |          |   HostSocket block at the card's base  |
 ---- ACNet, the card: Zorro II, Dalsin product 6, 64 KB -----------
      |          instance bridge on Linux: sockets, DNS, NetworkManager
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

- **Interface.** ACNet owns a checked-in classic Amiga BSD socket ABI manifest: 46 application vectors followed by 10 reserved growth slots. The vector table is generated from that manifest, not from a third-party stack SFD. Missing core calls return `ENOSYS` honestly.
- **Per opener.** Every `OpenLibrary()` returns a base of its own, following the classic Amiga socket model. Each base holds its own descriptors, errno, signals, h_errno and name buffers. Closing it closes its sockets.
- **Waiting.**
  - Blocking calls, WaitSelect and name lookups sleep on Exec signals. The device's interrupt wakes them; nothing polls.
  - Ctrl-C (or the opener's break mask) interrupts a wait with EINTR.
  - WaitSelect takes the caller's signal mask and a timeout (timer.device).
- **Name lookups** run on the host, asynchronously, so a slow DNS server never freezes the machine. gethostbyname, gethostbyaddr, getservbyname and getservbyport are answered by the host. Protocols come from a small built-in table.
- **Moving sockets between tasks.** ObtainSocket, ReleaseSocket, ReleaseCopyOfSocket and Dup2Socket work. Servers started inetd-style depend on them.
- **Interfaces and routes.** ACNet core does not expose another stack's interface/routing administration API. ACNetControl obtains configuration and status through ACNet's own control plane.
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

- A commodities.library broker with `CX_PRIORITY`, `CX_POPKEY` (default `ctrl alt n`) and `CX_POPUP`. Exchange's Show and Hide open and close the window. Kill quits the Commodity and never takes the network down.
- **Status:** online or off, the host link, the instance's address as seen from outside, DNS, sockets open, bytes in and out.
- **Wi-Fi:** the networks list (SSID, signal, security, known), Rescan, Join, Leave and Forget. It is greyed out without the permission.
- **Diagnostics:** look up a name, test a TCP connection to host:port, and the recent log.
- A window that fits a 640×256 Workbench, laid out by ReAction's layout.gadget, with clicktab for the pages.

## 7. The host service

- It runs in the instance bridge (`hostsocket.py`, beside `native_bridge.py`). Host sockets are non-blocking. The service never blocks the runtime longer than one non-blocking call.
- **Handles** are small integers per instance. Host descriptors are never exposed.
- **Events:** the guest's POLL names the sockets it waits on. A watcher thread posts one event, input record 11, when any of them, or a pending DNS answer, becomes ready. The runtime holds INT2 while the event is pending and IRQ_ENABLE is set.
- **Policy:**
  - Off unless the instance's switch is on; it is read again on every HELLO and SOCKET.
  - TCP and UDP over IPv4 only. Raw sockets are refused (EPERM), and so are binds below port 1024 (EACCES).
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
- Switching Network on fits the card from the next reboot and installs `LIBS:bsdsocket.library`, `DEVS:acnet.device`, `DEVS:acwifi.device` and `SYS:Tools/Commodities/ACNetControl` into an OS 3.2.3 instance's System volume. It does not overwrite an existing third-party `bsdsocket.library`; it reports the conflict instead.

## 10. Quality bar

- **Host:** unit tests for the service, covering policy, handles, POLL and events, DNS and errno mapping.
- **Board:** board_test cases for the block's registers, buffers and INT2.
- **Guest, under genuine AmigaOS 3.2.3:** a test program covering the library's lifecycle, many openers, TCP to a test server, UDP, WaitSelect with a timeout and a signal, Ctrl-C, name lookups, and ObtainSocket and ReleaseSocket.
- **Conformance:** bsdsocktest (tbdye, 142 tests). The target is everything attempted, no unexpected failures, and every limit written down.
- **Programs:** an FTP client, a browser, an IRC client, SimpleMail, and a long download.
- **Never:** busy polling, a wait that can't be broken, or a library open that can hang.

## 11. Limits in version 1

- Ping and traceroute need raw sockets. They will go through a host ping service, which is planned.
- Programs that drive SANA-II directly need acnet.device's SANA-II side, which is planned.
- IPv6 comes later.
- AROS 68k keeps its own stack. ACNet is built for OS 3.2.3 first.

## 12. Relationship to earlier designs

- **Kept:** the 30 September ACNet designs' interface, their test gates and their security boundary. Their two Commodities are merged into one.
- **Superseded:**
  - the 27 September ACNet v0.1 UDP-only stack;
  - the statement in the 27 September Network/Wi-Fi design that AmigaChrome would not supply bsdsocket.library;
  - that design's bridge-by-default rule (now off by default).
- **Still valid:** the ACNetwork (6) and ACWifiNet (8) board work, for the later packet-level card.

## 13. Order of work

1. The board block and the host service, with tests.
2. acnet.device and bsdsocket.library, to first light: an OS 3.2.3 program fetches a page with no TCP stack in the Amiga.
3. The Cradle switches and installing the files.
4. ACNetControl.
5. acwifi.device and the host Wi-Fi requests.
6. bsdsocktest and the program matrix (each download needs Dale's OK), then release.

## 14. Open

- Whether Wi-Fi may join networks the PC does not already know. That means passing a passphrase to the host once. Version 1 joins known networks only.
- The host ping service.
- When to build the SANA-II side of acnet.device.
