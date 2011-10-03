#!/bin/sh

insmod fsl_psc913x_hetmgr
mknod /dev/het_mgr c 252 0

exit 0
