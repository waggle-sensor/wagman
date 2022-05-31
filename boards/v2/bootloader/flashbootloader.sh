#!/bin/bash
# ANL:waggle-license
# This file is part of the Waggle Platform.  Please see the file
# LICENSE.waggle.txt for the legal details of the copyright and software
# license.  For more details on the Waggle project, visit:
#          http://www.wa8.gl
# ANL:waggle-license
avrdude -c avrisp2 -P usb -p m32u4 -U flash:w:Caterina-Micro.hex:i
