#!/bin/bash
# This file is part of the Waggle Platform.  Please see the file
# LICENSE.waggle.txt for the legal details of the copyright and software
# license.  For more details on the Waggle project, visit:
#          http://www.wa8.gl
echo "#define BUILD_TIME "$(echo $(date +%s)+12 | bc) > buildinfo.cpp
echo "#define BUILD_GIT \"$(git rev-parse HEAD | tail -c 5)\"" >> buildinfo.cpp
