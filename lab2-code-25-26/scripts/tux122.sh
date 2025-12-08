#!/bin/bash

sudo ifconfig if_e1 172.16.121.1/24

ping 172.16.121.1

# tux112

sudo route add -net  172.16.111.0/24 gw 172.16.111.253

sudo sysctl net.ipv4.conf.if_e1.accept_redirects=0
sudo sysctl net.ipv4.conf.all.accept_redirects=0

route del -net 172.16.110.0 gw 172.16.111.253 netmask 255.255.255.0

traceroute -n 172.16.50.1 # ver isso

route add -net 172.16.50.0/24 gw 172.16.51.253

traceroute -n 172.16.50.1 # ver isso dnv

sudo sysctl net.ipv4.conf.if_e1.accept_redirects=1
sudo sysctl net.ipv4.conf.all.accept_redirects=1