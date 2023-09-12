#!/bin/bash

set +x
set -Eeuo pipefail

echo "Enabling kernel module..."
cd /home/$USER/Desktop/m1-tools
cd kmod
sudo insmod kmod.ko

echo "Enabling dc civac..."
cd ../tools/chickenctl
sudo ./target/release/chickenctl

echo "Enabling cycle counter..."
cd ../pmuctl
sudo ./target/release/pmuctl

echo "Pinning CPU to max frequency..."
sudo cpupower frequency-set -g userspace
sudo cpupower -c 0-3 frequency-set -f 9999999
sudo cpupower -c 4-7 frequency-set -f 9999999

echo "Testing all of the above and taskset..."
cd /home/$USER/Desktop/asahi-cycle-counters

taskset -c 0 ./cycles_and_flush
taskset -c 1 ./cycles_and_flush
taskset -c 2 ./cycles_and_flush
taskset -c 3 ./cycles_and_flush
taskset -c 4 ./cycles_and_flush
taskset -c 5 ./cycles_and_flush
taskset -c 6 ./cycles_and_flush
taskset -c 7 ./cycles_and_flush
