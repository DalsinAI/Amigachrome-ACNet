#!/bin/sh
# Build acwifi.device, the product-6 ACNet Wi-Fi control facade.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
GUEST=$(cd "$HERE/../.." && pwd)
STOVE=${STOVE:-$HOME/AmigaChrome/stoves/os32}
CC="$STOVE/prefix/bin/m68k-amigaos-gcc"
OUT=${1:-$GUEST/build/guest/os32/acwifi}
mkdir -p "$OUT"

BARE="-m68020 -O2 -fomit-frame-pointer -fno-toplevel-reorder -fno-builtin -Wall -Wno-pointer-sign -nostartfiles -nostdlib"
INC="-I$GUEST/network/acnetwork/include -I$HERE/include"

"$CC" $BARE $INC -o "$OUT/acwifi.device" \
    "$HERE/device/acwifi_device.c" -lgcc
echo "$OUT/acwifi.device ($(wc -c < "$OUT/acwifi.device") bytes)"

if [ -f "$HERE/tests/acwifitest.c" ]; then
    "$CC" -m68020 -O2 -Wall -Wno-pointer-sign -noixemul $INC \
        -o "$OUT/acwifitest" "$HERE/tests/acwifitest.c"
    echo "$OUT/acwifitest ($(wc -c < "$OUT/acwifitest") bytes)"
fi
