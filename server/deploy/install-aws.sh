#!/bin/sh
BUILD_SCRIPT=./build-aws-graviton2.sh
SSH_ARGS=-i /path/to/key.pen
TARGET_HOST=username@ip.ad.dre.ss
TARGET_DIR=/home/ubuntu

$BUILD_SCRIPT
scp $SSH_ARGS ../mc4-server ../defs.json ../config_example.ini ../mc4-server-aws.service $TARGET_HOST:$TARGET_DIR/
make clean -C ..

ssh -t $SSH_ARGS $TARGET_HOST "cd $TARGET_DIR && cp -n config_example.ini config.ini && cat mc4-server-aws.service | sed s/TARGET_USER/$(whoami)/g | sed s|TARGET_DIR|$TARGET_DIR|g | sudo tee /etc/systemd/system/mc4-server.service"

echo "====="
echo "Installation complete. Please set TLS options in $TARGET_DIR/config.ini and run"
echo "  systemctl enable mc4-server"
echo "  systemctl start mc4-server"
