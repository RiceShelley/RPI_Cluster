#!/bin/bash
# bash script to maintain persistence of nodeManager on nodes in a distributed system
./compile.sh
while true; do
	# run nodeManager executable
	./bin/nodeManager
	# if node manager returns 0 restart node otherwise restart node manager 
	if [ "$?" -eq  "0" ]; then
		echo restarting client!!!
		exec shutdown -r now
	else 
		echo Unexpected error!!! Restarting node manager
	fi
done
