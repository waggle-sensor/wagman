#!/bin/sh
# This file is part of the Waggle Platform.  Please see the file
# LICENSE.waggle.txt for the legal details of the copyright and software
# license.  For more details on the Waggle project, visit:
#          http://www.wa8.gl

set -e
set -x

./reset-port.sh $1
sleep 5
avrdude -v -pm32u4 -cavr109 -P"$1" -b57600 -D -U"flash:w:$2:i"
