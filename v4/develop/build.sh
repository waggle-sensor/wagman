#!/bin/bash -e

sig=$(find firmware -type f | xargs sha1sum | sha1sum | awk '{print $1}')

docker run -it -v $PWD:/build firmware-builder bash -c "
    cd /build/firmware &&
    arduino-cli compile --warnings all -v -b arduino:sam:wagman_v2 -o bin/firmware-$sig"

echo "
built firmware $sig
"
