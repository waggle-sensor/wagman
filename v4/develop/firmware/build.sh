#!/bin/sh
# This file is part of the Waggle Platform.  Please see the file
# LICENSE.waggle.txt for the legal details of the copyright and software
# license.  For more details on the Waggle project, visit:
#          http://www.wa8.gl

arduino -v --board arduino:sam:arduino_due_x --verify firmware.ino
