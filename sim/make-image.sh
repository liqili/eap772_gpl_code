#!/bin/bash
# Make what the simulation needs (run it again after each firmware build):
#   eap772-fw-sim:latest   Docker image (linux/arm64) of a firmware rootfs
#   sim/nlshim.so          LD_PRELOAD shim for the missing TP-Link kernel modules (see nlshim.c)
# Usage, on the host:  sim/make-image.sh [SOURCE]
#   SOURCE defaults to the squashfs this tree last built. It can also be another squashfs image
#   (for example the rootfs volume of a flash dump) or a rootfs directory, such as the tree's
#   prebuilt eap_gpl_code_eap772/images/eap/ipq53xx_eap7xx_common/rootfs.
# The rootfs is converted with sim/sqfs2tar.py (python3, no unsquashfs needed); nlshim.so is
# cross-compiled in the build image (eap772_gpl_code-ubuntu) when it is missing or out of date.
set -eo pipefail
cd "$(dirname "$0")/.." || exit 1
BUILT=eap_gpl_code_eap772/images/eap/ipq53xx_eap7xx_common/ipq53xx_eap7xx_common-squashfs-root.img
SRC=${1:-$BUILT}
if [ ! -e "$SRC" ]; then
	echo "ERROR: $SRC not found."
	echo "Build the firmware first (see README \"Compile Steps\"), or pass a squashfs image or a rootfs directory, e.g.:"
	echo "  sim/make-image.sh eap_gpl_code_eap772/images/eap/ipq53xx_eap7xx_common/rootfs"
	exit 1
fi

echo "== import $SRC as eap772-fw-sim:latest"
python3 sim/sqfs2tar.py "$SRC" | docker import --platform linux/arm64 - eap772-fw-sim:latest
echo "arm64 emulation check (expect aarch64): $(docker run --rm --platform linux/arm64 eap772-fw-sim:latest /bin/busybox uname -m)"

if [ sim/nlshim.so -nt sim/nlshim.c ]; then
	echo "== sim/nlshim.so is up to date"
else
	echo "== build sim/nlshim.so"
	bash sim/build-nlshim.sh
fi
