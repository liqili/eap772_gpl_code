#!/bin/sh
# Cut-down EAP772/F76 rcS + rc.eapcs for a user-mode (qemu-aarch64) container: no firmware kernel,
# no flash, no TP-Link kernel modules, no Wi-Fi driver. Runs as the container's command, with this
# folder mounted read-only at /sim and the GPL's mkimage folder at /mkimage (see README
# "Simulating the firmware in Docker").
#   SIM_BOARD   F76_1_0_0 (default), EAP772_2_0 or EAP723_1_0: the unit the simulated flash holds
#               (its factory device-info, and the default-config.bin SIM_CONFIG=factory starts from)
#   SIM_CONFIG  factory (default): start from that board's mkimage default-config.bin, as make_flash
#               writes it for a new unit; none: start without a user-config, as after a reset
# Which board configuration (/etc/<board>) the firmware then runs is not SIM_BOARD's choice: as on a
# real unit, nvrammanager takes it from the image's own /fw_data/product-info. An image built from
# this tree says Festa F76 there (GPL_PRODUCT_BOARD in build/qca/Makefile), so it comes up as a Festa
# F76 (/etc/Festa_F76_1.0) whichever SIM_BOARD the simulated flash holds, exactly as on hardware; the
# tree's prebuilt rootfs still says EAP772.
log() { echo "[sim] $*"; }
sz() { ls -l "$1" 2>/dev/null | awk '{print $5}'; }	# this busybox has no wc or stat
BOARD=${SIM_BOARD:-F76_1_0_0}
MK=/mkimage/$BOARD
if [ ! -f "$MK/device-info.bin" ]; then
	log "ERROR: $MK/device-info.bin not found. SIM_BOARD must be F76_1_0_0, EAP772_2_0 or EAP723_1_0,"
	log "and the GPL mkimage folder must be mounted at /mkimage (docker-compose.yml does this)."
	exit 1
fi
log "board $BOARD: $(head -n 1 $MK/device-info.bin)"

export PATH=$PATH:/etc/ath
mkdir -p /var/run/srv /var/run/client /var/lock /var/run/lldpd /tmp/log/error /tmp/log/rec /tmp/log/coredump /tmp/logdump
chmod 777 /var/run/srv /var/run/client /var/lock /var/run/lldpd /tmp/logdump

# Fake flash, in place of mount_tpdata. On a unit, manu_data and backup_data are the factory volumes
# and user_data is the runtime config. device-info is the board's GPL device-info.bin; the region
# is US, the additional hardware version 0, and the MAC a made-up, locally administered
# 02:77:72:76:00:01. Sizes match a real unit's files.
mkdir -p /tp_data/manu_data /tp_data/backup_data /tp_data/user_data
M=/tp_data/manu_data
cp $MK/device-info.bin $M/device-info
printf '\002\167\162\166\000\001' > $M/default-mac
printf 'US840\000\000\000' > $M/region
printf '\000\000\000\000' > $M/add-hardver
cp $M/* /tp_data/backup_data/
# The runtime config is kept across container restarts and reset when the container is recreated.
case "${SIM_CONFIG:-factory}" in
factory)
	[ -f /tp_data/user_data/user-config ] || cp $MK/default-config.bin /tp_data/user_data/user-config
	log "user-config: $MK/default-config.bin ($(sz /tp_data/user_data/user-config) bytes)" ;;
none)
	log "user-config: $( [ -f /tp_data/user_data/user-config ] && echo "present, kept from an earlier start" || echo "none (as after a reset)")" ;;
*)
	log "ERROR: SIM_CONFIG must be factory or none"; exit 1 ;;
esac

log "nvrammanager -C"
nvrammanager -C
log "firmware board: $(cat /tmp/cfgFilePath 2>/dev/null) (from /fw_data/product-info: $(grep -m 1 'TP-Link' /fw_data/product-info))"
# device-info is not an nvram partition on this firmware (U-Boot reads it), so it is not listed
for p in product-info add-hardver region default-mac support-list soft-version; do
	nvrammanager -r /tmp/nm_$p -p $p >/dev/null 2>&1 && log "nvram $p: OK ($(sz /tmp/nm_$p) bytes)" || log "nvram $p: FAILED"
done

# TP-Link's private netlink families (portal.ko etc.) don't exist here: see /sim/nlshim.c
if [ -f /sim/nlshim.so ]; then
	export LD_PRELOAD=/sim/nlshim.so
	log "LD_PRELOAD=$LD_PRELOAD"
else
	log "WARNING: /sim/nlshim.so is missing (run sim/build-nlshim.sh on the host); uclited and the login will fail"
fi

for d in msg-center tpsyslogd; do log "start $d"; /usr/bin/$d & done
# uclited waits for the "<name>_ok" marker of these daemons once it has loaded the config
sh /sim/start-deps.sh
log "start uclited (auto-restart loop, log in /tmp/uclited-loop.log)"
sh /sim/uclited-loop.sh >> /tmp/uclited-loop.log 2>&1 &
# Under emulation uclited took about 12 minutes to get ready in testing
sh /sim/wait-ready.sh 1800 | sed 's/^/[sim] uclited: /'

# rc.eapcs starts the web server once uclited has created /tmp/ucliteok
log "start httpd_portal"
/usr/bin/httpd_portal &
sleep 5
log "processes:"; ps | grep -v "\["
log "listening:"; netstat -ltn 2>/dev/null
log "init done: web UI on port 443 (https://localhost:9443 with the default port mapping), admin/admin"
wait
