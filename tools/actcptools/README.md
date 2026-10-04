# ACTCPTools — AmigaOS 3.2.3 network utilities

ACTCPTools is OpenSocket's classic-Amiga command-line tool set (OpenSocket was ACNet until 4 October 2026). The tools install in `SYS:Tools/OpenSocket/`; `OpenSocket` (was `acnetctl`) installs in `C:`.

## Contract

The application-compatibility tools are ordinary AmigaOS programs using the published `bsdsocket.library` API:

`hostname/resolve/ping/traceroute/arp -> bsdsocket.library -> opensocket.library -> opensocket.device -> OpenSocket card -> Linux HostSocket`

The OpenSocket administration tools use the public `opensocket.library` API directly (it was `acnetwork.library`):

`ifconfig/route/netstat/OpenSocket -> opensocket.library -> opensocket.device -> OpenSocket card -> Linux HostSocket`

No ACTCPTool calls HostSocket, the device's private vectors, or AmigaChrome host APIs directly.

## Current port status

| Tool | OS3 build | ACNet runtime | Notes |
| --- | --- | --- | --- |
| `hostname` | PASS | PASS | End-to-end through ACNet `gethostname()`. |
| `resolve` | PASS | PASS | Forward lookup through ACNet name service. |
| `ping` | PASS | PASS | End-to-end through ACNet's constrained ICMP compatibility path. |
| `traceroute` | PASS | PASS | Raw-UDP guest probes are translated through Linux UDP/error-queue ICMP. |
| `arp` | PASS | PASS (read-only) | `arp -a -n` uses Roadshow `GetRouteInfo()` compatibility over ACNet neighbour enumeration. |
| `ifconfig` | PASS | PASS (read-only) | Native ACNet interface/address/link/counter view. |
| `route` | PASS | PASS (read-only) | Native ACNet IPv4 route view. |
| `netstat` | PASS | PASS (read-only) | Native ACNet state/counters and this-instance socket inventory. |
| `OpenSocket` (was `acnetctl`) | PASS | PASS | Native status and soft online/offline control; Cradle remains authoritative. |

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