#!/bin/sh
set -eu

HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
STOVE=${STOVE:-}
if [ -z "$STOVE" ]; then
    echo "Set STOVE to the AmigaChrome OS 3.2 cross-build stove." >&2
    exit 2
fi

CC="$STOVE/prefix/bin/m68k-amigaos-gcc"
OUT=${1:-"$HERE/build"}
CFLAGS="-m68020 -O2 -Wall -Wno-pointer-sign -noixemul -I$HERE/../../guest/network/acnetwork/include"
mkdir -p "$OUT"

build_one() {
    name=$1
    "$CC" $CFLAGS -I"$HERE/src" "$HERE/src/$name.c" -o "$OUT/$name"
    echo "$OUT/$name ($(wc -c < "$OUT/$name") bytes)"
}

build_one hostname
build_one resolve
build_one ping
build_one traceroute
build_one arp
build_one ifconfig
build_one route
build_one netstat
build_one acnetctl

echo "ACTCPTools build complete."