# ACNetControl Test Plan

ACNetControl is the ReAction Commodity front end for ACNet on AmigaOS 3.2.3.
Networking must continue to work with ACNetControl absent, hidden or killed.

## Gate 0 - Build

- Build with the OS3 stove as 68000 code.
- `file` must identify `ACNetControl` as an AmigaOS LoadSeg binary.
- Build must complete with no warnings.
- Keep the binary below 40 KB for version 1.

## Gate 1 - Basic launch

Use the disposable ACNet OS3 instance only.

- Start from CLI and Workbench.
- With ACNet fitted and online, Status reports Online.
- With Network off in Cradle, Status reports Off in Cradle.
- With no ACNet card, the application opens and explains the condition.
- Closing the window hides the Commodity and does not stop networking.

## Gate 2 - Commodity behaviour

- A second launch brings the existing copy to the front.
- Exchange: Show, Hide, Enable, Disable and Kill.
- `ctrl alt n` opens/brings the window forward.
- Ctrl-C exits a CLI-started test copy cleanly.
- No duplicate broker remains after Kill.

## Gate 3 - Visual/UI qualification

Capture screenshots from the native replay/frame tools at:

- 640x480 RTG Workbench;
- 640x256 Workbench;
- 640x200 Workbench.

Check all five pages: Status, Wi-Fi, Connections, Diagnostics and Log.
The window must remain font-sensitive/resizable and must not require MUI.
The screencast/reference design is the visual target: restrained native ReAction,
compact information density, and a persistent status line.

## Gate 4 - Existing ACNet path regression

With ACNetControl running and hidden:

- `acnettest` still passes hostname and DNS lookup;
- TCP loopback bind/listen/connect/accept/send/recv still passes;
- HostSocket unit suite remains green;
- no socket is closed merely because ACNetControl is hidden or killed.

## Gate 5 - Telemetry/control backend

After STATS, LOG and ONLINE are implemented in HostSocket/acnet.device:

- live socket count and traffic totals/rates;
- PC address/interface/gateway/DNS;
- soft Go offline / Go online;
- event log ring and sequence handling;
- hidden window waits on ACNet events rather than polling.

Cradle's Network switch remains authoritative and soft-offline never survives a reboot.

## Gate 6 - Connections and diagnostics

- Connections enumerates program/task, protocol, endpoints, state and byte totals.
- Version 1 remains read-only.
- DNS lookup and TCP connection tests only run when the user requests them.
- Check Internet performs no background/external contact.
- Copy report produces a plain-text support summary.

## Gate 7 - Wi-Fi

Use a mock NetworkManager only; automated tests must never change the real host Wi-Fi.

- permission-off page is greyed/explanatory;
- scan returns asynchronously;
- join/leave/forget require confirmation;
- version 1 only joins host-known networks;
- passphrases never transit the Amiga.

## Gate 8 - Robustness

- 100 Show/Hide cycles: no meaningful AvailMem loss.
- Kill frees all resources.
- repeated launch/unique notification does not create a second copy.
- no-card and non-ACNet bsdsocket.library cases remain informative, not fatal.
