# RCOM Project 2 Report

[Title, Authors, Summary]

# Introduction

# Part 1

The developed application is present in "client/ftp_client.c"

Compilation: 

``` bash
gcc ftp_client.c -o ftp_client 
```

Execution:

``` bash
./ftp_client ftp://[user:password@]host/path
```

# Part 2

## Experience 1 - Configure an IP Network

### Step 1

E1 of tux123 and E1 of tux124 were connected to ether21 and ether23 respectively, in the switch.

### Step 2

To configure each of the tuxes, the "ifconfig if_e1 [ipaddress]" command was used to setup the interface.

* tux123:

``` bash
sudo ifconfig if_e1 172.16.120.1/24 # setup interface if_e1
```

* tux124:

``` bash
sudo ifconfig if_e1 172.16.120.254/24 # setup interface if_e1
```

### Step 3

The MAC and IP address of each tux can be obtained using the "ifconfig" command. They are as following:

* tux123:

``` bash
MAC: ec:75:0c:c2:3c:ac
IP: 172.16.120.1
```

* tux124:

``` bash
MAC: ec:75:0c:c2:3c:75
IP: 172.16.120.254
```

### Step 4

Using the "ping [ipaddress]" command, we can verify the connectivity of the tuxes between eachother:

* tux123:

``` bash
ping 172.16.120.254 # ping tux124
```

* tux124:

``` bash
ping 172.16.120.1 # ping tux123
```

### Step 5

On each of the tuxes:

``` bash
sudo route -n # shows forwarding table entries

sudo arp -a # shows arp table entries
```

### Step 6

* tux123:

``` bash
sudo arp -d 172.16.120.1 # deletes arp table entries
```

### Steps 7, 8, 9 and 10

First, we start the capture on tux123.if_e1 with Wireshark.

Then, we ping tux124 from tux123 for a few seconds:

``` bash
ping 172.16.120.254 # ping tux124
```

The Wireshark log is available in "wslogs/exp1/exp1_log.pcapng".

## Questions: Experiment 1

### What are the ARP packets and what are they used for?

### What are the MAC and IP addresses of ARP packets and why?

### What packets does the ping command generate?

### What are the MAC and IP addresses of the ping packets?

### How to determine if a receiving Ethernet frame is ARP, IP, ICMP?

### How to determine the length of a receiving frame?

### What is the loopback interface and why is it important?

---

## Experience 2 - Implement two bridges in a switch

### Step 1

E1 of tux122 was connected to ether19 in the switch. In the terminal:

``` bash
sudo ifconfig if_e1 172.16.121.1/24 # setup if_e1
```

It's MAC and IP address are as follows:

``` bash
MAC: ec:75:0c:c2:31:73
IP: 172.16.121.1
```

### Step 2

To create the bridges, we access the MicroTik terminal in GTKTerm on tux122. After logging in, we resetted the configurations so the switch was clean:

``` bash
system reset-config
```

Then, the following commands were used to setup the bridges:

``` bash
/interface bridge add name=bridge120
/interface bridge add name=bridge121
```

### Step 3

Inside GTKTerm, the following commands were executed:

1. Removing the tuxes:

``` bash
/interface bridge port remove [find interface =ether19] # tux122
/interface bridge port remove [find interface =ether21] # tux123
/interface bridge port remove [find interface =ether23] # tux124
```

2. Adding them to the bridges:

``` bash
/interface bridge port add bridge=bridge121 interface=ether19 # tux122

/interface bridge port add bridge=bridge120 interface=ether21 # tux123
/interface bridge port add bridge=bridge120 interface=ether23 # tux124
```

### Steps 4, 5 and 6

After initiating Wireshark in tux123.if_e1, we ping tux124 and tux122 from tux123 for a few seconds:

``` bash
ping 172.16.120.254 # ping tux124

ping 172.16.121.1 # ping tux122
```

Important: the ping to tux122 FAILS, as tux123 is not in the same subnet as tux122.

The Wireshark log is available in "wslogs/exp2/exp2_log1.pcapng"

### Steps 7, 8 and 9

After initiating captures in the corresponding tuxes, we ping broadcast from tux123 for a few seconds:

``` bash
ping -b 172.16.120.255 # ping broadcast
```

The Wireshark logs are available in "wslogs/exp2/exp2_log2_tux22.pcapng", "wslogs/exp2/exp2_log2_tux23.pcapng" and "wslogs/exp2/exp2_log2_tux24.pcapng".

### Step 10

Steps 7, 8 and 9 are repeated, but for tux122.

The Wireshark logs are available in "wslogs/exp2/exp2_log3_tux22.pcapng", "wslogs/exp2/exp2_log3_tux23.pcapng" and "wslogs/exp2/exp2_log3_tux24.pcapng".

## Questions

### How to configure bridgeY0?

### How many broadcast domains are there? How can you conclude it from the logs?

---

## Experience 3 - Configure a Router in Linux

### Step 1

To configure tux124 as a router, first we connected the E2 of tux124 to ether22.

In GTKTerm, we configure the interface:

``` bash
interface bridge port remove [find interface=ether22]
interface bridge port add bridge=bridge121 interface=ether22
```

Then, the following commands were used:

``` bash
sudo ifconfig if_e2 172.16.121.253/24 # setup interface if_e2

sudo sysctl -w net.ipv4.ip_forward=1 # enable ip forwarding

sudo sysctl -w net.ipv4.icmp_echo_ignore_broadcasts=0 # disable icmp e-i-b broadcasts
```

### Step 2

The MAC and IP addresses of tux124 are as following:

* if_e1:

``` bash
MAC: ec:75:0c:c2:3c:75
IP: 172.16.120.254
```

* if_e2:

``` bash
MAC: ec:75:0c:c2:10:6b
IP: 172.16.121.253
```

### Step 3

For the tuxes to reach eachother, they need a route through tux124. The following commands set that up:

``` bash
sudo route add -net 172.16.120.0/24 gw 172.16.121.253 # for tux122
 
sudo route add -net 172.16.121.0/24 gw 172.16.120.254 # for tux123
```

Note: "gw" stands for gateway. The specified gateway should be accessible by the tux.

### Step 4

On tuxes 122, 123 and 124, execute:

``` bash
sudo route -n # observe routes and confirm if they're tux-accessible
```

### Steps 5, 6 and 7

After initiating capture in tux123, we ping each interface for a few seconds:

``` bash
ping 172.16.120.254 # ping tux124.if_e1

ping 172.16.121.253 # ping tux124.if_e2

ping 172.16.121.1 # ping tux122
```

The Wireshark log is available in "wslogs/exp3/exp3_log1.pcapng".

### Steps 8, 9, 10 and 11

After initiating 2 captures in tux124.if_e1 and tux124.if_e2 respectively, we clean each tux's arp tables:

``` bash
sudo arp -d 172.16.121.1 # remove arp table entries in tux122

sudo arp -d 172.16.120.1 # remove arp table entries in tux123

sudo arp -d 172.16.120.254 # remove arp table entries in tux124
```

Then we ping tux122 from tux123 for a few seconds:

``` bash
ping 172.16.121.1 # ping tux122
``` 

The Wireshark log are available in "wslogs/exp3/exp3_log2_e1.pcapng" and "wslogs/exp3/exp3_log2_e2.pcapng".

## Questions

### What routes are there in the tuxes? What are their meaning?

### What information does an entry of the forwarding table contain?

### What ARP messages, and associated MAC addresses, are observed and why?

### What ICMP packets are observed and why?

### What are the IP and MAC addresses associated to ICMP packets and why?

---

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

traceroute to 172.16.120.1 (172.16.120.1), 30 hops max, 60 byte packets  # result
 1  172.16.121.253  0.211 ms  0.194 ms  0.177 ms
 2  172.16.120.1  0.408 ms  0.391 ms  0.375 ms
```

Activating the acceptance of ICMP redirect:

```bash
sudo sysctl net.ipv4.conf.if_e1.accept_redirects=1
sudo sysctl net.ipv4.conf.all.accept_redirects=1
```
If we delete the route via tux124, tux122 will not be able to reach the subnet 172.16.120.0, as the route through the router has been deleted.

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


---

## Experience 5 - DNS

### Step 1

To add DNS to each tux, we need to edit the file "/etc/resolv.conf":

```bash
sudo vim /etc/resolv.conf
```

Then, we add the following line:

```bash
nameserver 10.227.20.3
```

### Step 2

To test if DNS is working, we can ping google:

```bash
ping www.google.com # ping google
```

### Step 3

Wireshark will show the DNS query and response.

The Wireshark log is available in "wslogs/exp5/googleping.pcapng".

## Questions

### How to configure the DNS service in a host?

### What packets are exchanged by DNS and what information is transported?

---

# Experience 6 - TCP connections

### Steps 1, 2, 3 and 4

To compile the client:

``` bash
gcc ftp_client.c -o ftp_client
```

To test the client, we compiled it then tried downloading the file "pipe.txt" from the ftp server:

``` bash
./ftp_client ftp://ftp.netlab.fe.up.pt/pipe.txt
```

The downloaded file is in "wslogs/exp6/pipe.txt"

The Wireshark log is available in "wslogs/exp6/ftptest.pcapng"

### Step 5

For step 5, the Wireshark logs are available in "wslogs/exp6/downloadtux2.pcapng" and "wslogs/exp6/downloadtux3.pcapng".

## Questions

### How many TCP connections are opened by your FTP application?

### In what connection is transported the FTP control information?

### What are the phases of a TCP connection?

### How does the ARQ TCP mechanism work? What are the relevant TCP fields?

### What relevant information can be observed in the logs?

### How does the TCP congestion control mechanism work? What are the relevant fields. How did the throughput of the data connection evolve along the time? Is it according to the TCP congestion control mechanism?

### Is the throughput of a TCP data connections disturbed by the appearance of a second TCP connection? How?

---

# Conclusions 

# References

# Annexes
