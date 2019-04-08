#!/bin/bash
# This file is part of the Waggle Platform.  Please see the file
# LICENSE.waggle.txt for the legal details of the copyright and software
# license.  For more details on the Waggle project, visit:
#          http://www.wa8.gl
echo "#define CURRENT_TIME "$(echo $(date +%s)+12 | bc) > rtc_conf.cpp
echo "#define SET_RTC_TIME" >> rtc_conf.cpp
cat rtc_conf.cpp
