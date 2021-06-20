#!/bin/sh
BUILD_SCRIPT=./build-aws-graviton2.sh
SSH_ARGS=-i /path/to/key.pen
TARGET_HOST=username@ip.ad.dre.ss
TARGET_DIR=/home/ubuntu

$BUILD_SCRIPT
ssh $SSH_ARGS $TARGET_HOST "systemctl stop mc4-server"
scp $SSH_ARGS ../mc4-server ../defs.json $TARGET_HOST:$TARGET_DIR/
ssh $SSH_ARGS $TARGET_HOST "systemctl start mc4-server"
make clean -C ..

echo "====="
echo "Update and restart on $TARGET_HOST complete."
echo "  systemctl enable mc4-server"
echo "  systemctl start mc4-server"
