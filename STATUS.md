# ACNet Status

## 2026-10-01 - First Light

Guest proof on AmigaOS 3.2.3:

- library open: PASS
- hostname: PASS (`instance-6`)
- DNS localhost: PASS
- TCP loopback bind/listen/connect/accept: PASS
- send/recv payload verification: PASS
- guest smoke program return code: 0

Build outputs at this checkpoint:

- `acnet.device`: 3,628 bytes
- `bsdsocket.library`: 20,560 bytes
- `acnettest`: 5,208 bytes
- generated ABI: 139 vector slots
- implemented vectors: 46
- generated stubs: 75

Platform evidence retained in the AmigaChrome repository:

- HostSocket host tests: 20/20 pass
- ACNet-enabled native A1200 board tests: 89/89 pass

Next qualification targets:

1. `WaitSelect` readiness and signal semantics.
2. UDP send/receive.
3. `FIONBIO`, blocking/non-blocking transitions, and timeout behaviour.
4. `getsockopt`/`setsockopt` coverage.
5. DNS, service, and address conversion edge cases.
6. Real-world Amiga TCP/IP application compatibility.
7. Add optional legacy-stack compatibility only where real applications require it.
