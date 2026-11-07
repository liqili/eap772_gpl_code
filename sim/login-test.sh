#!/bin/bash
# Send one admin login to the simulated device (https://localhost:9443) the way login.html does,
# then fetch / with the session cookie. Run on the host: sim/login-test.sh [password]
# Responses are saved in logs/sim/.
cd "$(dirname "$0")/.." || exit 1
OUT=logs/sim
mkdir -p "$OUT"
B=${SIM_URL:-https://localhost:9443}
PW=${1:-admin}
HASH=$(printf %s "$PW" | md5 -q 2>/dev/null || printf %s "$PW" | md5sum | cut -d' ' -f1)
HASH=$(echo "$HASH" | tr a-f A-F)
J=$OUT/cookies.txt
: > "$J"
H=(-H "Referer: $B/" -H "X-Requested-With: XMLHttpRequest")
echo "== POST / (login as admin)"
curl -sk -m 30 -c "$J" -b "$J" "${H[@]}" -D "$OUT/login-headers.txt" -o "$OUT/login-body.txt" \
	-d username=admin -d "password=$HASH" "$B/"
cat "$OUT/login-headers.txt"; head -c 400 "$OUT/login-body.txt"; echo
# curl writes HttpOnly cookies as "#HttpOnly_<host> ..." lines; the comment lines start with "# "
echo "== cookies"; grep -v '^# ' "$J" | grep .
echo "== GET / with the cookie (first 300 bytes)"
curl -sk -m 30 -c "$J" -b "$J" "${H[@]}" "$B/" | head -c 300; echo
echo "== uclited"
docker exec eap772-sim sh -c 'ps | grep [/]usr/bin/uclited || echo "uclited is NOT running"'
