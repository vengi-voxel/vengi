#!/bin/bash

if [[ $(id -u) -ne 0 ]]; then
	echo "Please run as root"
	exit
fi

APP_PID=`pidof $1`
perf record -F 99 -p ${APP_PID} -g -- sleep 120
perf report -n --stdio
