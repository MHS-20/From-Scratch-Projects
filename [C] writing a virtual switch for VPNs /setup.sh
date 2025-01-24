#!/bin/bash
ip tuntap add dev mytap mode tap
ip addr add 10.1.1.101/24 dev mytap
ip link set mytap up