#!/bin/sh

msg=`dmesg | grep "fsl_shm"`
major=`echo $msg | cut -d: -f2`

# Now test if special device
# is created, if not create it
if [ ! -c /dev/fsl_shmem ]
then
		echo "Creating special device..."
		mknod /dev/fsl-shmem c $major 0
		echo "done"
fi

exit 0
