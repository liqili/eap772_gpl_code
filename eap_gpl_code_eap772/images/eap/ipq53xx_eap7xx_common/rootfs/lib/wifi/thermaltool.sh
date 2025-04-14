#!/bin/sh

if [ $1 == wifi0 ]; then
	echo "==== configure 2G thermal policy ===="
	thermaltool -i wifi0 -set -lo0 -100
	thermaltool -i wifi0 -set -hi0 110
	thermaltool -i wifi0 -set -off0 0

	thermaltool -i wifi0 -set -lo1 105
	thermaltool -i wifi0 -set -hi1 115
	thermaltool -i wifi0 -set -off1 20

	thermaltool -i wifi0 -set -lo2 110
	thermaltool -i wifi0 -set -hi2 120
	thermaltool -i wifi0 -set -off2 40

	thermaltool -i wifi0 -set -lo3 115
	thermaltool -i wifi0 -set -hi3 125
	thermaltool -i wifi0 -set -off3 80
	
elif [ $1 == wifi1 ]; then
	echo "==== configure 5G thermal policy ===="
	thermaltool -i wifi1 -set -lo0 -100
	thermaltool -i wifi1 -set -hi0 109
	thermaltool -i wifi1 -set -off0 0

	thermaltool -i wifi1 -set -lo1 108
	thermaltool -i wifi1 -set -hi1 113
	thermaltool -i wifi1 -set -off1 40

	thermaltool -i wifi1 -set -lo2 110
	thermaltool -i wifi1 -set -hi2 116
	thermaltool -i wifi1 -set -off2 70

	thermaltool -i wifi1 -set -lo3 112
	thermaltool -i wifi1 -set -hi3 120
	thermaltool -i wifi1 -set -off3 90
	
elif [ $1 == wifi2 ]; then
	echo "==== configure 6G thermal policy ===="
	thermaltool -i wifi2 -set -lo0 -100
	thermaltool -i wifi2 -set -hi0 110
	thermaltool -i wifi2 -set -off0 0

	thermaltool -i wifi2 -set -lo1 105
	thermaltool -i wifi2 -set -hi1 115
	thermaltool -i wifi2 -set -off1 40

	thermaltool -i wifi2 -set -lo2 107
	thermaltool -i wifi2 -set -hi2 117
	thermaltool -i wifi2 -set -off2 70

	thermaltool -i wifi2 -set -lo3 110
	thermaltool -i wifi2 -set -hi3 120
	thermaltool -i wifi2 -set -off3 90
	
fi
