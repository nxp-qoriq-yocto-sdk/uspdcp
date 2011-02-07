#!/bin/bash


LOG_FILE=test_sec_log.txt
#ARCH=powerpc
ARCH=i686

TEST_PROGRAM_PATH=../../../../bin-$ARCH



if [ -e $LOG_FILE ]
then
    rm $LOG_FILE
fi


for i in `seq 1000`
do
    echo "Iteration number $i" >> test_sec_log.txt
    $TEST_PROGRAM_PATH/test_sec_driver_poll >> test_sec_log.txt

    if [[ $? != 0 ]] ; then
        echo "Iteration number $i generated error!!" >> test_sec_log.txt
        exit $?
    fi

done

echo 'SEC test done' >> test_sec_log.txt
echo 'SEC test done'
