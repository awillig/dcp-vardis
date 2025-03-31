#!/usr/bin/bash

if [ "$#" -ne 1 ]; then
    echo "One parameter expected: numerical short hand node index.";
    exit;
fi

SHORTHAND=$1
let AVERAGE="$SHORTHAND * 1000"
let AVERAGEX="$SHORTHAND * 1000 + 150"
let AVERAGEY="$SHORTHAND * 1000 + 300"
let AVERAGEZ="$SHORTHAND * 1000 + 450"

sudo rm /tmp/dcp-* > /dev/null 2>&1
sudo rm /dev/shm/shm-area* > /dev/null 2>&1
sudo rm /dev/shm/shm-vardis* > /dev/null 2>&1
sudo rm log-dcp-* > /dev/null 2>&1

start_bp() {
    sudo ../_build/dcp-bp -r cfg/bp/simple-test.cfg & > /dev/null 2>&1
    BPPID=$!
    echo "Started BP demon, pid is $BPPID"
}


stop_bp() {
    echo "Killing (sudo) BP demon with pid $BPPID"
    sudo kill -9 $BPPID
}


start_vd() {
    ../_build/dcp-vardis -r cfg/vardis/simple-test.cfg  & > /dev/null 2>&1
    VDPID=$!
    echo "Started Vardis demon, pid is $VDPID"
}

stop_vd() {
    echo "Killing Vardis demon with pid $VDPID"
    kill -INT $VDPID
}

start_srp() {
    ../_build/dcp-srp -r cfg/srp/simple-test.cfg  & > /dev/null 2>&1
    SRPPID=$!
    echo "Started SRP demon, pid is $SRPPID"
}

stop_srp() {
    echo "Killing SRP demon with pid $SRPPID"
    kill -INT $SRPPID
}

start_var_prod() {
    ../_build/vardisapp-test-producer $SHORTHAND 250 $AVERAGE 100  & > /dev/null 2>&1
    PRODPID=$!
    echo "Started Vardis producer for varId $SHORTHAND, pid is $PRODPID"
}

stop_var_prod() {
    echo "Killing Vardis variable producer with pid $PRODPID"
    kill -INT $PRODPID
}

start_pos_gen() {
    ../_build/srpapp-test-generate-sd 150 $AVERAGEX $AVERAGEY $AVERAGEZ 100  & > /dev/null 2>&1
    SGENPID=$!
    echo "Started SRP test generator, pid is $SGENPID"
}

stop_pos_gen() {
    echo "Killing SRP test position generator with pid $SGENPID"
    kill -INT $SGENPID
}

cleanup() {
    sudo rm /tmp/dcp-* > /dev/null 2>&1
    sudo rm /dev/shm/shm-area* > /dev/null 2>&1
    sudo rm /dev/shm/shm-vardis* > /dev/null 2>&1
}


start_bp
sleep 1
start_vd
sleep 1
start_srp
sleep 1
start_var_prod
sleep 1
start_pos_gen
sleep 1

echo "Press a key to stop node. Logs will be preserved."
read -n1
echo "Exiting."

stop_pos_gen
sleep 1
stop_var_prod
sleep 1
stop_srp
sleep 1
stop_vd
sleep 1
stop_bp
sleep 1
cleanup

exit;
