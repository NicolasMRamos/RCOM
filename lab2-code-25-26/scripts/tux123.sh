#!/bin/bash

sudo ifconfig if_e1 172.16.120.1/24

ping 172.16.120.1

# tux113

route add -net 172.16.110.0/24 gw 172.16.110.254
