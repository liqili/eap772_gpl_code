# cliclientd
var=$0
name=${var##*/}
tool=${name#*x}
arg=$*
if [[ $tool = 'ping' ]] || [[ $tool = 'tcpdump' ]]
then
	if [[ $# -eq 1 ]] && [[ $1 = 'stop' ]]
	then
		tool=$tool$1
		arg=""
	else
		tool=$tool"start"
	fi
fi
cliclientd $tool "$arg"
