# OpenSocket

OpenSocket is a free (MIT) `bsdsocket.library` for AmigaOS 3.2.3: the classic Amiga socket interface, its core library, the card's driver, a Commodity and the standard network tools. It was called ACNet until 4 October 2026. The repository is `DalsinAI/openamigasocket` (it was `DalsinAI/Amigachrome-ACNet`; GitHub forwards the old address).

Today it has one backend, OpenSocket Host: socket calls travel through the OpenSocket card (Zorro II, Dalsin product 6) in an AmigaChrome machine to real sockets on the PC. OpenSocketDirect, a TCP/IP stack of its own (lwIP) for real Amigas and PiStorm, is planned. The design is `docs/architecture/OPENSOCKET_DESIGN.md` in DalsinAI/amigachrome.

## The parts

| File | Installed as | What it is |
| --- | --- | --- |
| `bsdsocket.library` | `LIBS:` | The published socket interface: the 46 classic vectors (none stubbed) and 27 Roadshow-era calls after them (BPF, routes, modern IPv4 lookups). Its id string says OpenSocket. |
| `opensocket.library` | `LIBS:` | OpenSocket's core: status, interfaces, routes, sockets, the event log, Wi-Fi requests. Everything else opens it. Was `acnetwork.library`. |
| `opensocket.device` | `DEVS:Networks/` | The card's driver: private calls for sockets, and a SANA-II face for frames. Was `acnet.device`; programs that open it fall back to `acnet.device` on older installs. |
| `opensocketwifi.device` | `DEVS:` | Control of the PC's Wi-Fi, by permission. Was `acwifi.device`. |
| `OpenSocketControl` | `SYS:Tools/Commodities/` | The Commodity, in GadTools. Was ACNetControlGT; the ReAction ACNetControl is retired. |
| `OpenSocket` | `C:` | Status, online, offline. Was `acnetctl`. |
| ping, traceroute, arp, ifconfig, route, netstat, hostname, resolve | `SYS:Tools/OpenSocket/` | The standard network tools (ACTCPTools). Never put in `C:`, so another stack's commands are not overwritten. |

The network interface is `opensocket0` (was `acnet0`). The card's HostSocket block keeps its signature ("ACH1"), commands and product number, so the wire format is unchanged.

## Building

The NDK is not redistributed here. Set `STOVE` to an AmigaOS 3.x cross-build environment containing `m68k-amigaos-gcc` and NDK 3.2 (`$STOVE/ndk`, for the SANA-II and Roadshow headers).

```sh
STOVE=/path/to/os32 ./guest/network/acnet/build.sh ./build/opensocket
STOVE=/path/to/os32 ./tools/actcptools/build.sh ./build/opensocket
```

The first builds the libraries, both devices, `OpenSocketControl` and the qualification programs (acnettest, bsdqual and the probes); the second builds `OpenSocket` and the tools. The builds are reproducible: the same commit gives the same bytes.

The vector table is generated from the checked-in manifest `guest/network/acnet/abi/bsdsocket-v4.json`; no third-party socket-library SFD is read. Source folders still carry their ACNet-era names (`guest/network/acnet`, `acnetwork`, `acwifi`); they move with the repository rename.

## Status

The status log from 1 to 3 October is in `docs/history/STATUS.md`. The complete stack builds and passes its host-side tests; live qualification of the matched guest and host set on AmigaOS 3.2.3 comes next. Until then, treat it as alpha.

## AmigaChrome integration

The AmigaChrome runtime supplies the OpenSocket card, the HostSocket service on the PC, the Network switch and the installer that puts this payload on an instance. Those live in DalsinAI/amigachrome with the machine they depend on.

## Licence and credit

OpenSocket is MIT-licensed: `LICENSE`, Copyright (c) 2026 Dalsin Limited. Anyone may use, change, fork and redistribute it, commercially or not. The one condition is the MIT one: the copyright and permission notice stays with every copy and every fork.

A request, not a condition: if you fork or ship OpenSocket, please say it is based on OpenSocket by Dalsin Limited.

Inside the project, a few files keep their own licences:
- `tools/actcptools/src/ping.c`, `traceroute.c` and `arp.c` derive from 4.4BSD-Lite2 (through Olaf Barthel's Amiga ports) and keep the University of California's licence and notices (`tools/actcptools/THIRD_PARTY.md`).
- lwIP, when OpenSocketDirect brings it in, keeps its own BSD licence and notice.

AmigaOS/NDK material is not included and remains subject to its own licensing terms.

## Contributors

OpenSocket is created and maintained by [SacredTrees](https://github.com/SacredTrees) with the AmigaChrome agent team, copyright Dalsin Limited. Everyone whose work it includes is credited in [`CONTRIBUTORS.md`](CONTRIBUTORS.md).
