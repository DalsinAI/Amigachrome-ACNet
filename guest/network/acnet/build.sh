#!/bin/sh
# ACNet for AmigaOS 3.2.3: acnet.device, native acnetwork.library and
# bsdsocket.library compatibility facade (bare: no startup code or C library), plus the test
# program acnettest (libnix), with the os32 stove (bebbo's m68k-amigaos-gcc,
# NDK 3.2). The library vector table is generated from ACNet's own classic
# Amiga BSD socket ABI manifest; no third-party socket SFD is required.
#   network/acnet/build.sh [OUT_DIR]     (default build/guest/os32/acnet)
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
GUEST=$(cd "$HERE/../.." && pwd)
if [ -z "${STOVE:-}" ]; then
  echo "Set STOVE to an AmigaOS 3.x cross-build environment containing m68k-amigaos-gcc and standard NDK headers." >&2
  exit 2
fi
CC="$STOVE/prefix/bin/m68k-amigaos-gcc"
ABI="$HERE/abi/bsdsocket-v4.json"
GUARD="$HERE/compat/roadshow/guard-v4.json"
OUT=${1:-$GUEST/build/guest/os32/acnet}
mkdir -p "$OUT"
BARE="-m68020 -O2 -include sys/types.h -fomit-frame-pointer -fno-toplevel-reorder -fno-builtin -Wall -Wno-pointer-sign -nostartfiles -nostdlib"
INC="-I$GUEST/common/protocol -I$HERE/include -I$GUEST/network/acnetwork/include"
COMPAT_INC="-I$STOVE/ndk/SANA+RoadshowTCP-IP/include"

"$CC" $BARE $INC -o "$OUT/acnet.device" "$HERE/device/acnet_device.c" "$GUEST/common/os3/string.c" -lgcc
echo "$OUT/acnet.device ($(wc -c < "$OUT/acnet.device") bytes)"

STOVE="$STOVE" "$GUEST/network/acnetwork/build.sh" "$OUT"

L="$HERE/library"
# lib_base.c first: its start() must be the library's first code.
LIB="$L/lib_base.c $L/lib_fd.c $L/lib_errno.c $L/lib_tags.c $L/lib_strings.c $L/lib_select.c $L/lib_conn.c $L/lib_io.c $L/lib_opt.c $L/lib_names.c $L/lib_inet.c $HERE/compat/roadshow/routes.c $HERE/compat/roadshow/state.c"
python3 "$HERE/library/gen_vectors.py" "$ABI" "$OUT/bsdsocket_vectors.c" $LIB --guard "$GUARD"
"$CC" $BARE $INC $COMPAT_INC -I"$HERE/library" -I"$OUT" -o "$OUT/bsdsocket.library" \
    $LIB "$HERE/library/provider_acnetwork.c" "$OUT/bsdsocket_vectors.c" "$GUEST/common/os3/string.c" -lgcc
echo "$OUT/bsdsocket.library ($(wc -c < "$OUT/bsdsocket.library") bytes)"

if [ -f "$HERE/tests/acnettest.c" ]; then
    "$CC" -m68020 -O2 -Wall -noixemul -o "$OUT/acnettest" "$HERE/tests/acnettest.c"
    echo "$OUT/acnettest ($(wc -c < "$OUT/acnettest") bytes)"
fi

if [ -f "$HERE/tests/bsdqual.c" ]; then
    "$CC" -m68020 -O2 -Wall -Wno-pointer-sign -noixemul -o "$OUT/bsdqual" "$HERE/tests/bsdqual.c"
    echo "$OUT/bsdqual ($(wc -c < "$OUT/bsdqual") bytes)"
fi

if [ -f "$HERE/tests/roadshow_readonly_probe.c" ]; then
    "$CC" -m68020 -O2 -Wall -Wno-pointer-sign -noixemul \
        -o "$OUT/roadshow-readonly-probe" "$HERE/tests/roadshow_readonly_probe.c"
    echo "$OUT/roadshow-readonly-probe ($(wc -c < "$OUT/roadshow-readonly-probe") bytes)"
fi

if [ -f "$HERE/control/acnetcontrol.c" ]; then
    "$CC" -m68000 -O2 -Wall -Wno-pointer-sign -noixemul -I"$HERE/include" -I"$GUEST/network/acnetwork/include" \
        -o "$OUT/ACNetControl" "$HERE/control/acnetcontrol.c" -lamiga
    echo "$OUT/ACNetControl ($(wc -c < "$OUT/ACNetControl") bytes)"
fi
