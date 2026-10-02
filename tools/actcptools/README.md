# ACTCPTools — AmigaOS 3.2.3 network utilities

ACTCPTools is the classic-Amiga command-line tool set used to qualify ACNet.

## Contract

The tools are ordinary AmigaOS programs. They use the published `bsdsocket.library` API only:

`ACTCPTools -> bsdsocket.library -> acnetwork.library -> acnet.device -> ACNet card -> Linux HostSocket`

They do not call HostSocket, ACNet private vectors, or AmigaChrome host APIs directly.

## Current port status

| Tool | OS3 build | ACNet runtime | Notes |
| --- | --- | --- | --- |
| `hostname` | PASS | PASS | End-to-end through ACNet `gethostname()`. |
| `resolve` | PASS | PASS | Forward lookup through ACNet name service. |
| `ping` | PASS | PASS | End-to-end through ACNet's constrained ICMP compatibility path. |
| `traceroute` | PASS | PASS | Raw-UDP guest probes are translated through Linux UDP/error-queue ICMP. |
| `arp` | PASS | PASS (read-only) | `arp -a -n` uses Roadshow `GetRouteInfo()` compatibility over ACNet neighbour enumeration. |

All binaries target 68020 and AmigaOS 3.2.3.
## Building

Use the same placed OS 3.2.3 NDK and m68k-amigaos GCC stove as ACNet:

```sh
STOVE=/path/to/os32 ./tools/actcptools/build.sh
```

The build does not fetch an NDK or networking SDK.

## Porting changes

`ping` and `traceroute` started from Olaf Barthel's classic-Amiga ports of the 4.4BSD-Lite2 utilities shipped with the NDK networking material.

The AmigaChrome port removes their floating-point RTT calculations and uses integer microseconds instead. This avoids a dependency on `mathieeedoubbas.library` and keeps the tools suitable for a plain 68020 guest.

`hostname` and `resolve` are small ACNet BSD-3-Clause programs written specifically for this package.

`arp` currently remains source-compatible with the supplied Amiga port, but ACNet intentionally has no mutable guest ARP table in HostSocket mode.