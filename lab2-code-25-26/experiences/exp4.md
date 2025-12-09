# Experience 4 - Configure a Commercial Router and Implement NAT

### Step 1

First, we connect ether1 of the Router to PY.24 (Or PY.12, if that doesn't work).

Then, we connect ether2 of the Router to ether15 of the switch, and configure it in GTKTerm:

``` bash
interface bridge port remove [find interface = ether15]
interface bridge port add bridge=bridge121 interface=ether15
```

After configuring the switch, we switch the MicroTik terminal to the Router's console to configure it's IP addresses:

``` bash
ip address add interface=ether1 address=10.227.20.129/24
ip address add interface=ether2 address=172.16.121.254/24
```

### Step 2

Verifying routes:

* tux122:

``` bash
sudo route add -net 172.16.120.0/24 gw 172.16.121.253 # reaches tux123

sudo route add -net 172.16.1.0/24 gw 172.16.121.254 # reaches FTP server
```

* tux123:

``` bash
sudo route add -net 172.16.121.0/24 gw 172.16.120.254 # reaches Router and tux122

sudo route add -net 172.16.1.0/24 gw 172.16.120.254 # reaches FTP server (through tux124)
```

* tux124:

``` bash
sudo route add -net 172.16.1.0/24 gw 172.16.121.254 # reaches FTP server
```

* Router (in MicroTik terminal):

``` bash
ip route add dst-address=172.16.120.0/24 gateway=172.16.121.253
```

### Step 3

Using the following commands, we pinged tuxes 122, 124 and the Router:

``` bash
ping 172.16.121.1 # tux122

ping 172.16.20.254 # tux124

ping 172.16.21.254 # Router
```

The Wireshark log is available in "wslogs/exp1/exp4_log1.pcapng".

### Step 4

Used commands:

``` bash
sudo sysctl net.ipv4.conf.if_e1.accept_redirects=0
sudo sysctl net.ipv4.conf.all.accept_redirects=0

route del -net 172.16.110.0 gw 172.16.111.253 netmask 255.255.255.0

traceroute -n 172.16.50.1

route add -net 172.16.50.0/24 gw 172.16.51.253

traceroute -n 172.16.50.1

sudo sysctl net.ipv4.conf.if_e1.accept_redirects=1
sudo sysctl net.ipv4.conf.all.accept_redirects=1
```

### Step 5


### Step 6


### Step 7

## Questions

### How to configure a static route in a commercial router?

### What are the paths followed by the packets, with and without ICMP redirect enabled, in the experiments carried out and why?

### How to configure NAT in a commercial router?

### What does NAT do?

### What happens when tuxY3 pings the FTP server with the NAT disabled? Why?


