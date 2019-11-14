#!/bin/sh
# This file is part of the Waggle Platform.  Please see the file
# LICENSE.waggle.txt for the legal details of the copyright and software
# license.  For more details on the Waggle project, visit:
#          http://www.wa8.gl

if [[ "$(uname)" == "Darwin" ]]; then
  echo not implemented yet
  exit 1
else
  ls /dev/serial/by-id/*Arduino*Micro* | sed -n '1p'
fi
