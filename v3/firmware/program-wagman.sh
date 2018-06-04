#!/bin/sh

set -e
set -x

./reset-port.sh $1
sleep 5
avrdude -v -pm32u4 -cavr109 -P"$1" -b57600 -D -U"flash:w:$2:i"
