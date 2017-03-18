#!/bin/sh

if [[ "$(uname)" == "Darwin" ]]; then
  echo not implemented yet
  exit 1
else
  ls /dev/serial/by-id/*Arduino*Micro* | sed -n '1p'
fi
