#!/bin/bash

sudo ifconfig if_e1 172.16.120.254/24

sudo ping 172.16.120.254

# tux114

sudo ifconfig if_e2 172.16.110.253/24

sudo sysctl -w net.ipv4.ip_forward=1

sudo sysctl -w net.ipv4.icmp_echo_ignore_broadcasts=0
