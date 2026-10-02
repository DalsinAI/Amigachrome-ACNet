# Optional Roadshow compatibility

ACNet's default `bsdsocket.library` is intentionally a clean classic-Amiga BSD
socket implementation.  It does not model Roadshow's routing, interface,
monitoring, BPF, mbuf, kernel-memory, server, global-data or IP-filter APIs.

If application evidence later justifies one of those extensions it belongs in
this directory as an explicit compatibility module/build profile.  It must not
become an internal dependency of the ACNet socket core, HostSocket provider,
`acnet.device`, or ACNetControl.

`usergroup.library` is likewise not part of the ACNet core.  If a particular
legacy runtime requires it, treat it as an optional application-compatibility
package with its own tests and release boundary.
