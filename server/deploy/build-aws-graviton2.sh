#!/bin/sh
make clean "$@"
CXX=aarch64-linux-gnu-g++ CPPFLAGS="-DTLS -march=armv8.2-a+fp16+rcpc+dotprod+crypto -mtune=cortex-a72" make "$@"
