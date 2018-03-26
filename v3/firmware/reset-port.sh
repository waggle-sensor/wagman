#!/bin/sh

if [[ "$(uname)" == "Darwin" ]]; then
  stty -f $1 1200 hupcl
else
  stty -F $1 1200 hupcl
fi
