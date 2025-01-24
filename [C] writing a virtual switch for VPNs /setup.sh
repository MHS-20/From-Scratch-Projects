#!/bin/bash

ip addr add 10.1.1.101/24 dev mytap
ip link set mytap up

gcc -I. -c tap_utils.c -o tap_utils.o
gcc -I. -c vPort.c -o vPort.o
gcc -o vport vPort.o tap_utils.o -lpthread

gcc -c vSwitch.c -o vSwitch.o
gcc -o vswitch vSwitch.o