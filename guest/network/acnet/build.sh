#!/bin/sh
# ACNet for AmigaOS 3.2.3: acnet.device and bsdsocket.library (bare: no
# startup code or C library; each ROMTag's stub comes first), and the test
# program acnettest (libnix), with the os32 stove (bebbo's m68k-amigaos-gcc,
# NDK 3.2). The library's vector table is generated from NDK 3.2's
# bsdsocket_lib.sfd at build time, so the NDK stays out of git.
#   network/acnet/build.sh [OUT_DIR]     (default build/guest/os32/acnet)
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
GUEST=$(cd "$HERE/../.." && pwd)
if [ -z "${STOVE:-}" ]; then
  echo "Set STOVE to an AmigaOS 3.x cross-build environment containing m68k-amigaos-gcc and the NDK 3.2 Roadshow SFD." >&2
  exit 2
fi
CC="$STOVE/prefix/bin/m68k-amigaos-gcc"
SFD="$STOVE/ndk/SANA+RoadshowTCP-IP/sfd/bsdsocket_lib.sfd"
OUT=${1:-$GUEST/build/guest/os32/acnet}
mkdir -p "$OUT"
BARE="-m68020 -O2 -include sys/types.h -fomit-frame-pointer -fno-toplevel-reorder -fno-builtin -Wall -Wno-pointer-sign -nostartfiles -nostdlib"
INC="-I$GUEST/common/protocol -I$HERE/include"

"$CC" $BARE $INC -o "$OUT/acnet.device" "$HERE/device/acnet_device.c" "$GUEST/common/os3/string.c" -lgcc
echo "$OUT/acnet.device ($(wc -c < "$OUT/acnet.device") bytes)"

L="$HERE/library"
# lib_base.c first: its start() must be the library's first code.
LIB="$L/lib_base.c $L/lib_fd.c $L/lib_errno.c $L/lib_tags.c $L/lib_strings.c $L/lib_select.c $L/lib_conn.c $L/lib_io.c $L/lib_opt.c $L/lib_names.c $L/lib_inet.c"
python3 "$HERE/library/gen_vectors.py" "$SFD" "$OUT/bsdsocket_vectors.c" $LIB
"$CC" $BARE $INC -I"$HERE/library" -I"$OUT" -o "$OUT/bsdsocket.library" \
    $LIB "$HERE/library/provider_hostsocket.c" "$OUT/bsdsocket_vectors.c" "$GUEST/common/os3/string.c" -lgcc
echo "$OUT/bsdsocket.library ($(wc -c < "$OUT/bsdsocket.library") bytes)"

if [ -f "$HERE/tests/acnettest.c" ]; then
    "$CC" -m68020 -O2 -Wall -noixemul -o "$OUT/acnettest" "$HERE/tests/acnettest.c"
    echo "$OUT/acnettest ($(wc -c < "$OUT/acnettest") bytes)"
fi
