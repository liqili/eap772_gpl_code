#!/bin/bash
# Cross-compile sim/nlshim.so (aarch64 musl) with the firmware toolchain, in the build image
# (eap772_gpl_code-ubuntu, see docker-compose.yml; built first if it is missing). Uses the
# toolchain that eap_prep unpacks into eap_gpl_code_eap772/build/toolchain if it is there;
# otherwise unpacks eap_gpl_code_eap772/toolchain/*.tar.bz2_a? inside the container (about
# 300 MB, a minute or two), so the tree is left untouched.
# sim/make-image.sh runs this. Run on the host: sim/build-nlshim.sh
set -eo pipefail
cd "$(dirname "$0")/.." || exit 1
ROOT=$(pwd)
IMAGE=eap772_gpl_code-ubuntu
if ! docker image inspect "$IMAGE" >/dev/null 2>&1; then
	echo "== build the build image $IMAGE"
	if command -v docker-compose >/dev/null 2>&1; then docker-compose build ubuntu; else docker compose build ubuntu; fi
fi
docker run --rm -v "$ROOT/eap_gpl_code_eap772:/build:ro" -v "$ROOT/sim:/sim" "$IMAGE" bash -c '
set -e
P=toolchain-aarch64_cortex-a73_gcc-7.5.0_musl
T=/build/build/toolchain/$P
if [ ! -x $T/bin/aarch64-openwrt-linux-musl-gcc ]; then
	echo "unpacking $P in the container (eap_prep has not unpacked it in the tree)"
	mkdir -p /tmp/tc && cat /build/toolchain/$P.tar.bz2_a? | tar -xjf - -C /tmp/tc
	T=/tmp/tc/$P
fi
STAGING_DIR=$T $T/bin/aarch64-openwrt-linux-musl-gcc -shared -fPIC -O2 -Wall -o /sim/nlshim.so /sim/nlshim.c -ldl
ls -l /sim/nlshim.so'
