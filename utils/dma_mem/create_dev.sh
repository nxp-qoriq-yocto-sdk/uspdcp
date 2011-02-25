#!/bin/sh

if [ "x$1" = "x" ]; then
	echo "usage: ./create_dev.sh {char_device_name}"
        exit 1
fi
major=10
minor=`grep $1 /proc/misc | awk "{print \\$1}"`
echo "create /dev/$1 $major $minor"
mknod /dev/$1 c $major $minor
if [ $? -eq 0 ]; then
    echo "device /dev/$1 created successfully."
else
    echo "mknod failed to create device /dev/$1. exit code = $?."
fi
exit 0
