#!/bin/bash -e

sig=$(git ls-files -s firmware | awk '/\.cpp|\.h/' | git hash-object --stdin)

docker run -it -v $PWD:/build firmware-builder bash -c "
    cd /build/firmware &&
    arduino-cli compile --warnings all -v -b arduino:sam:wagman_v2 -o bin/firmware-$sig"

echo "
built firmware $sig
"
