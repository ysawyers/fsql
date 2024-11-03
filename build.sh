#!/bin/bash

if ! test -d build; then
    mkdir build;
fi

debug=false
[[ "$*" =~ ^-debug$ ]] && debug=true

if $debug; then
    cmake -DDEBUG_MODE=1 -B build
else
    cmake -DDEBUG_MODE=0 -B build
fi

cd build
make