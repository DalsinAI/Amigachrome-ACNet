# ACNet — acnetwork.library native boundary

**Date:** 2 October 2026

## Purpose

`bsdsocket.library` remains the classic Amiga BSD-socket compatibility facade. Its descriptor table, errno behaviour, tag handling, WaitSelect semantics, netdb compatibility and published v4 vector layout remain compatibility policy.

`acnetwork.library` sits below that facade and owns the native ACNet provider/session boundary:

```text
classic application
       |
bsdsocket.library
       |
acnetwork.library
       |
acnet.device
       |
ACNet card / HostSocket
       |
Linux networking
```

AmigaChrome-aware software may later open `acnetwork.library` directly without being constrained by the legacy bsdsocket ABI.

## v1 native vectors

- `ACNetwork_Call()` — one provider request/response
- `ACNetwork_Arm()` — arm the opener's event waiter
- `ACNetwork_Disarm()` — disarm that waiter
- `ACNetwork_SignalMask()` — signal bit used by WaitSelect/native callers
- `ACNetwork_LastError()` — most recent provider error
- `ACNetwork_State()` — card/online state

Each `OpenLibrary()` receives an independent session with its own `acnet.device` open and event signal.

## ICMP compatibility

`bsdsocket.library` now recognises protocol `icmp`/1. A classic application can request `socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)` while ACNet's HostSocket safely virtualises that operation. The Linux host does not receive unrestricted raw-socket capability.

For ICMP Echo the host uses Linux's unprivileged datagram ICMP socket. HostSocket restores the guest ICMP identifier, recomputes the ICMP checksum, and prepends a synthetic IPv4 header so the classic Amiga application sees conventional raw-socket receive semantics.

All other raw protocols remain refused.

## Qualification

On Instance-6 (AmigaOS 3.2.3 ACNet test guest):

- existing BSD compatibility qualification plus ICMP name/number tests: **49 PASS / 0 FAIL**
- ACTCPTools `ping -c 1 -t 2 127.0.0.1`: **RC 0, 0% loss**
- observed loopback RTT after EClock conversion fix: **0.324 ms**

The BSD vector ABI was unchanged by insertion of `acnetwork.library`.
