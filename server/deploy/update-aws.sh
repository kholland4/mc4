#!/bin/bash
BUILD_SCRIPT=./build-aws-graviton2.sh
SSH_ARGS="-i /path/to/key.pem"
TARGET_HOST=ec2-user@ip.ad.dre.ss
TARGET_DIR=/home/ec2-user

set -eE

function onexit {
  echo "*** Failed; mc4-server may not have been restarted."
}
trap onexit ERR

$BUILD_SCRIPT -C .. -j12
echo "Stopping server..."
ssh $SSH_ARGS $TARGET_HOST "sudo systemctl stop mc4-server"
scp $SSH_ARGS ../mc4-server ../defs.json $TARGET_HOST:$TARGET_DIR/
echo "Starting server..."
ssh $SSH_ARGS $TARGET_HOST "sudo systemctl start mc4-server"
make clean -C ..

echo
echo
echo "====="
echo "Update and restart on $TARGET_HOST complete."
