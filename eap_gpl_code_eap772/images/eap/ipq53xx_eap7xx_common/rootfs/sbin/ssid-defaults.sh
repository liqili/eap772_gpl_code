#!/bin/sh
# Give the default SSIDs a per-unit suffix, the way a factory-flashed unit has one.
#
# /etc/<board>/default_config is the configuration applied when there is no stored one (a factory
# reset, or a freshly flashed build). It ships with plain names, the same on every unit, because a
# file in the firmware cannot carry a per-unit MAC. This rewrites those names with the unit's MAC
# suffix before uclited reads them, so a defaulted device looks like a factory one:
#
#     EAP772_2.4GHz  ->  EAP772_2.4GHz_D5A720
#
# The suffix is the last three bytes of the device MAC, plus 0 for 2.4 GHz, 1 for 5 GHz and 3 for
# 6 GHz, matching the names in mkimage/<board>/default-config.json.
#
# rcS runs this just before uclited, once the flash and the network interfaces are up. Every step
# is optional: if the MAC cannot be read, or the file cannot be replaced, the shipped names are
# left alone and the device still has a working SSID. Nothing here changes a stored configuration,
# so it has no effect on a device that is already set up.
#
# The firmware does contain a name generator (wlan_lib_setDefEssid in libwlan_lib.so), but it is
# not applied when the default configuration is loaded: an empty ssidutf8 stays empty.

BOARD=$(cat /tmp/cfgFilePath 2>/dev/null)
CFG="$BOARD/default_config"
TMPF=/tmp/default_config.ssid
log() { echo "(ssid-defaults): $*" > /dev/console; }

[ -n "$BOARD" ] && [ -f "$CFG" ] || { log "no board config ($CFG), leaving defaults"; exit 0; }

# The MAC, as text. /sys/class/net/<if> is readable by this point; the factory copy in
# /tp_data/manu_data/default-mac is six raw bytes, which this shell has no tool to read as hex.
IFACE=eth0
MAC=$(cat /sys/class/net/$IFACE/address 2>/dev/null)
case "$MAC" in
*:*:*:*:*:*) ;;
*) log "no MAC from /sys/class/net/$IFACE/address, leaving defaults"; exit 0 ;;
esac

# Last three bytes -> 24-bit number, then one suffix per radio (2.4 GHz, 5 GHz, 6 GHz).
# The conversion is written out because this busybox's awk has no strtonum (it crashes on it).
SUFFIXES=$(echo "$MAC" | awk -F: '
	function hex2num(s,   i, c, d, n) {
		n = 0
		s = toupper(s)
		for (i = 1; i <= length(s); i++) {
			d = index("0123456789ABCDEF", substr(s, i, 1)) - 1
			if (d < 0) return -1
			n = n * 16 + d
		}
		return n
	}
	{
		n = hex2num($4 $5 $6)
		if (n < 0) exit 1
		printf "%06X %06X %06X", n % 16777216, (n + 1) % 16777216, (n + 3) % 16777216
	}' 2>/dev/null)
set -- $SUFFIXES
[ $# -eq 3 ] || { log "could not derive a suffix from $MAC, leaving defaults"; exit 0; }
log "MAC $MAC -> suffixes $1 $2 $3"

# Append the suffix to the first three ssidutf8 names, unless one is already there.
awk -v s1="$1" -v s2="$2" -v s3="$3" '
	BEGIN { n = 0 }
	/"ssidutf8"/ && n < 3 {
		if (match($0, /"ssidutf8":[ \t]*"/)) {
			head = substr($0, 1, RSTART + RLENGTH - 1)
			rest = substr($0, RSTART + RLENGTH)
			q = index(rest, "\"")
			if (q > 1) {
				val = substr(rest, 1, q - 1)
				tail = substr(rest, q)
				suf = (n == 0 ? s1 : (n == 1 ? s2 : s3))
				hex = "[0-9A-Fa-f]"
				if (val !~ ("_" hex hex hex hex hex hex "$"))
					val = val "_" suf
				$0 = head val tail
			}
		}
		n++
	}
	{ print }
' "$CFG" > "$TMPF" 2>/dev/null

# A truncated or empty result would leave the device with no usable default, so check first.
if [ ! -s "$TMPF" ] || [ "$(grep -c '"ssidutf8"' "$TMPF" 2>/dev/null)" != "3" ]; then
	log "rewrite produced an unusable file, leaving defaults"
	rm -f "$TMPF"
	exit 0
fi

# Put it where uclited looks. libutility_lib builds that path from the board name, so the file
# itself has to be replaced: a bind mount on the read-only rootfs of a real device, or a plain
# copy where /etc happens to be writable (the Docker simulation). Either is fine; neither is
# required.
if mount -o bind "$TMPF" "$CFG" 2>/dev/null; then
	log "bind-mounted the per-unit defaults over $CFG"
elif cp -f "$TMPF" "$CFG" 2>/dev/null; then
	log "copied the per-unit defaults to $CFG"
else
	log "could not replace $CFG (read-only and no bind mount), leaving defaults"
	exit 0
fi
log "default SSIDs: $(grep '"ssidutf8"' "$CFG" | tr -d ' \t' | tr '\n' ' ')"
exit 0
