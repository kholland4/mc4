#!/bin/sh
make clean "$@"
CXX=x86_64-linux-gnu-g++ CPPFLAGS="-DTLS" make "$@"
