# Amigachrome-ACNet

ACNet is the Amiga-side networking project developed for AmigaChrome. It provides a `bsdsocket.library` compatible front end, `acnet.device`, a provider boundary, and the HostSocket guest protocol used to reach modern host networking services.

The public repository contains the reusable guest networking project. AmigaChrome-specific machine emulation, native bridge integration, Cradle UI/configuration, and instance lifecycle code remain in the main AmigaChrome repository.

## Current status

Verified through 2 October 2026 with AmigaOS 3.2.3 in the dedicated AmigaChrome ACNet test instance:

- `bsdsocket.library` opens successfully.
- `gethostname()` succeeds.
- DNS lookup of `localhost` succeeds.
- TCP `socket`, `bind`, `listen`, `connect`, `accept`, `send`, and `recv` succeed end to end.
- The guest loopback payload is verified and returns RC 0.
- The logical core exposes the classic 46-vector application ABI plus 10 permanently reserved slots; 43 vectors are implemented and 3 are honest `ENOSYS` stubs.
- The physical table is padded safely through slot 139 with compatibility guards, so callers built for a larger legacy socket ABI fail cleanly instead of jumping beyond the library. These guard slots are not ACNet APIs.

The clean-core guest qualification passes 47/47 with RC 0. The corresponding AmigaChrome HostSocket service has 21/21 host tests passing, and the ACNet-enabled native A1200 board has 89/89 board tests passing. Those host/runtime tests live in the AmigaChrome repository because they are platform-specific.

## Repository layout

- `guest/network/acnet/library/` - `bsdsocket.library` front end and provider interface.
- `guest/network/acnet/device/` - `acnet.device` guest driver for the ACNet card transport.
- `guest/network/acnet/include/` - private guest device interface.
- `guest/common/protocol/` - shared ACNet/HostSocket protocol definitions.
- `guest/common/os3/` - minimal OS3 support used by the bare binaries.
- `guest/network/acnet/tests/acnettest.c` - first-light AmigaOS smoke test.
- `docs/` - architecture and compatibility design material.

## Building

The NDK is deliberately not redistributed in this repository. Set `STOVE` to an AmigaOS 3.x cross-build environment containing `m68k-amigaos-gcc` and the standard AmigaOS networking headers. ACNet owns its ABI manifest; no third-party socket-library SFD is required.

```sh
STOVE=/path/to/os32 ./guest/network/acnet/build.sh ./build/acnet
```

A successful build produces:

```text
acnet.device
bsdsocket.library
acnettest
bsdqual
ACNetControl
ACNetControlGT
```

The vector table is generated from ACNet's checked-in `guest/network/acnet/abi/bsdsocket-v4.json` manifest. No third-party socket-library SFD is read by the default build. The compatibility guard layout is a checked-in safety manifest under `guest/network/acnet/compat/`.

## Scope

ACNet is currently an alpha implementation. The near-term compatibility campaign covers `WaitSelect`, UDP, non-blocking I/O, socket timeouts/options, name/service lookups, and normal Amiga TCP/IP applications. Stack-specific administration APIs are outside the core contract.

## AmigaChrome integration

The AmigaChrome runtime supplies the ACNet Zorro-II card, HostSocket host service, interrupt wiring, network enable/disable policy, and Cradle lifecycle. Those components are intentionally maintained with the machine/runtime they depend on rather than duplicated here.

## License

The guest-side ACNet source in this repository is BSD-3-Clause. See `LICENSE`.

AmigaOS/NDK material is not included and remains subject to its own licensing terms.
