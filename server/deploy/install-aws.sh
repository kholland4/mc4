#!/bin/bash
BUILD_SCRIPT=./build-aws-graviton2.sh
SSH_ARGS="-i /path/to/key.pem"
TARGET_HOST=ec2-user@ip.ad.dre.ss
TARGET_DIR=/home/ec2-user

set -e

$BUILD_SCRIPT -C .. -j12
scp $SSH_ARGS ../mc4-server ../defs.json ../config_example.ini mc4-server-aws.service $TARGET_HOST:$TARGET_DIR/
make clean -C ..

ssh $SSH_ARGS $TARGET_HOST "sudo yum install -y openssl11"
ssh $SSH_ARGS $TARGET_HOST "cd $TARGET_DIR && cp -n config_example.ini config.ini && cat mc4-server-aws.service | sed s/TARGET_USER/\$(whoami)/g | sed s#TARGET_DIR#$TARGET_DIR#g | sudo tee /etc/systemd/system/mc4-server.service"

echo
echo
echo "====="
echo "Installation complete."
echo "Please set TLS and other options in $TARGET_DIR/config.ini and run"
echo "  sudo systemctl enable mc4-server"
echo "  sudo systemctl start mc4-server"
echo "  sudo systemctl status mc4-server"

ssh $SSH_ARGS $TARGET_HOST
