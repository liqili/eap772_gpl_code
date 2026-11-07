#!/bin/sh
# Inside eap772-sim: wait until uclited has loaded the config and created /tmp/ucliteok (what
# rc.eapcs waits for before starting the web server). Usage: sh /sim/wait-ready.sh [max-seconds]
max=${1:-600}; i=0
while [ ! -d /tmp/ucliteok ] && [ $i -lt "$max" ]; do sleep 2; i=$((i+2)); done
if [ -d /tmp/ucliteok ]; then echo "ready after ${i}s"; else echo "NOT ready after ${i}s"; exit 1; fi
