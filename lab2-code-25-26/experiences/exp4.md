# Experience 4 - Configure a Commercial Router and Implement NAT

### Step 1

First, we connect ether1 of the Router to PY.24 (Or PY.12, if that doesn't work) and configure it in the Microtik terminal:

``` bash
ip address add interface=ether1 address=172.16.1.121/24 # interface for ftp server
```

Then, we connect ether2 of the Router to ether15 of the switch, and configure it in GTKTerm:

``` bash
interface bridge port remove [find interface = ether15]
interface bridge port add bridge=bridge121 interface=ether15
```

After configuring the switch, we configure the interface in the Microtik terminal:

``` bash
ip address add interface=ether2 address=172.16.121.254/24 # interface for tux123
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
ip route add dst-address=172.16.120.0/24 gateway=172.16.121.253 # for tux123 
```

### Step 3

Using the following commands, we pinged tuxes 122, 124 and the Router:

``` bash
ping 172.16.121.1 # tux122

ping 172.16.120.254 # tux124

ping 172.16.121.254 # Router
```

The Wireshark log is available in "wslogs/exp1/exp4_log1.pcapng".

### Step 4

``` bash
sudo sysctl net.ipv4.conf.if_e1.accept_redirects=0
sudo sysctl net.ipv4.conf.all.accept_redirects=0
```

To change tux122's routes, we need to delete the existing one and add the one through the router:

``` bash
sudo route del -net 172.16.120.0 gw 172.16.121.253 netmask 255.255.255.0 # delete if_e2 route

sudo route add -net 172.16.120.0/24 gw 172.16.121.254 # add Router route
```

After initiating capture in tux122, we ping tux123:

``` bash
ping 172.16.120.1 # ping tux123
```
The Wireshark log is available in "wslogs/exp1/exp4_log2.pcapng".

Traceroute command and log:

``` bash
traceroute -n 172.16.120.1 # command

traceroute to 172.16.120.1 (172.16.120.1), 30 hops max, 60 byte  packets # result
 1  172.16.121.254  0.165 ms  0.167 ms  0.170 ms
 2  172.16.121.253  0.361 ms  0.351 ms  0.342 ms
 3  172.16.120.1  0.602 ms  0.570 ms  0.551 ms
```
Readding tux124 as gateway:

``` bash
sudo route del -net 172.16.120.0 gw 172.16.121.254 netmask 255.255.255.0 # delete Router route

sudo route add -net 172.16.120.0/24 gw 172.16.121.253 # add if_e2 route
```
Doing traceroute again:

```bash
traceroute -n 172.16.120.1 # command

traceroute to 172.16.20.1 (172.16.20.1), 30 hops max, 60 byte packets  # result
 1  172.16.121.253  0.211 ms  0.194 ms  0.177 ms
 2  172.16.120.1  0.408 ms  0.391 ms  0.375 ms
```

Activating the acceptance of ICMP redirect:

```bash
sudo sysctl net.ipv4.conf.if_e1.accept_redirects=1
sudo sysctl net.ipv4.conf.all.accept_redirects=1
```
If we delete the route via tux124, tux122 will not be able to reach the subnet 172.16.20.0, as the route through the router has been deleted.

### Steps 5, 6 and 7

We ping the FTP server from tux123 using:

```bash
ping 172.16.1.10 # ping FTP server
```
If NAT is on in the Router, we can verify the connectivity between tux123 and Router.

If NAT is turned off, then when we try to ping the FTP server from tux123, we will get an error, because the FTP server does not know how to reach tux123 in the way back.

## Questions

### How to configure a static route in a commercial router?

### What are the paths followed by the packets, with and without ICMP redirect enabled, in the experiments carried out and why?

### How to configure NAT in a commercial router?

### What does NAT do?

### What happens when tuxY3 pings the FTP server with the NAT disabled? Why?


