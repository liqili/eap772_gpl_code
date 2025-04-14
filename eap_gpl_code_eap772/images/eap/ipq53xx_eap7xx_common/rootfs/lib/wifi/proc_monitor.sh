#!/bin/sh
######## configure #########
debug=0
#interval to check each process
check_intval=10

#process list need to periodly check 
#read from config file
#proc 	check_args_or_not 	args
#hostapd 	0	-
#httpd 		1	-p
config_file=/etc/proc_monitor.config
#plist=hostapd eap-cs httpd 'httpd -p'

#if thre is no config file use default configure
[ -f ${config_file} ] || {
config_file=/tmp/proc_monitor.config
cat > ${config_file} << EOF
hostapd 	0	-
EOF
}

######## configure end #########
info () {
	[ "${debug}" = "1" ] && echo $@
}

proc_alive() {
	alive=0

	# get the process pid list of $1
	tmp=`pgrep $1`
	pid_list=`echo ${tmp}`

	[ -n "${pid_list}" ] || {
		return ${alive}
	}

	for pid in $pid_list
	do 
		#need to check proc args match?
		[ "$2" = "0" ] || {
			cmdline=`cat /proc/${pid}/cmdline | grep -a ""`
			args=`echo "${cmdline}" | head -n2 | tail -1`
			info "args:${args} proc args:${3}"
			[ -z "$3" ] && [ -n "${args}" ] && continue
			[ -n "$3" ] && [ "${args}" != "$3" ] && continue
		}

		if [ "$2" = "1" ]; then
			info "[monitor] $1 $3 <$pid> alive"
		else
			info "[monitor] $1 <$pid> alive"
		fi

		alive=1
	done
	
	return ${alive}
}

restart_proc() {
	echo "[monitor] Going to restart <$1>"
	case $1 in
		hostapd)
			#reconfigure vaps
			xdebug hostapd recovery
			;;
		*)
			info "[monitor] TODO for restarting <$1>"
			;;
	esac
}

check_proc_alive() {
	while read -r proc check args ; do
		[ -z "${proc}" ] && continue

		info "proc:${proc} check:${check} args:${args}"

		proc_alive ${proc} ${check} ${args}
		[ $? = "0" ] && {
			restart_proc ${proc} ${args}
		}
		
	done < ${config_file}
}

while true ;  do

	check_proc_alive

	sleep ${check_intval};
done
