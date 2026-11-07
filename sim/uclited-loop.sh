#!/bin/sh
# Inside eap772-sim: keep uclited running, restarting it a second after it exits.
# sim-init.sh starts this loop; `docker exec eap772-sim sh /sim/uclited-ctl.sh restart` restarts it.
# Before each start the readiness marker is removed, so a wait for it waits for this uclited, and
# a socket a killed uclited left behind (/var/run/srv/6) is moved aside so the new one can bind it.
export LD_PRELOAD=/sim/nlshim.so
n=0
while :; do
	n=$((n+1))
	rm -rf /tmp/ucliteok
	[ -S /var/run/srv/6 ] && mv -f /var/run/srv/6 /tmp/.srv6.stale
	echo "[uclited-loop] start #$n at $(date +%T)"
	/usr/bin/uclited
	echo "[uclited-loop] uclited #$n exited with $? at $(date +%T)"
	sleep 1
done
