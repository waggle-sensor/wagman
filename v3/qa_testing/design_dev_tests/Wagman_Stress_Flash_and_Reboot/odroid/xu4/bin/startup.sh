#!/bin/bash
# This file is part of the Waggle Platform.  Please see the file
# LICENSE.waggle.txt for the legal details of the copyright and software
# license.  For more details on the Waggle project, visit:
#          http://www.wa8.gl
sleep 15
curl advaitha.pw/startingstress/node?=$(hostname)
sleep 5
stress-ng --cpu 8 --io 2 --vm 1 --vm-bytes 500M --timeout 180s --metrics-brief
sleep 5
curl advaitha.pw/endstress/node?=$(hostname)
sleep 2
shutdown -h now
