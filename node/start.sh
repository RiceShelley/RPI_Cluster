#!/bin/bash
# bash script to maintain persistence of nodeManager on nodes in a distributed system
cd /home/alarm/node && ./compile.sh
while true; do
	# run nodeManager executable
	cd /home/alarm/node && ./bin/nodeManager
	# if node manager returns 0 restart node otherwise restart node manager 
	if [ "$?" -eq "0" ]; then
		echo restarting client!!!
		exec shutdown -r now
	else 
		if [ "$?" -eq "1" ]; then
			echo shutting down client!!!
			exec shutdown now
		else
			echo Unexpected error!!! Restarting node manager
		fi
	fi
done
