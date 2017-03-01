#!/bin/bash
for ip in $( cat ipOfNodes.txt ); do
	echo starting ssh terminal for node $ip
	xfce4-terminal -e "sshpass -p 'root' ssh root@$ip" -T "$ip"
done
