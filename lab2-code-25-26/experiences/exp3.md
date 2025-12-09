# Experience 3 - Configure a Router in Linux

### Step 1

To configure tux124 as a router, first we connected the E2 of tux124 to ether22.

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
MAC: ec:75:0c:c2:tbd:tbd
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

The Wireshark log is available in "wslogs/exp3/exp3_log2.pcapng".

## Questions

### What routes are there in the tuxes? What are their meaning?

### What information does an entry of the forwarding table contain?

### What ARP messages, and associated MAC addresses, are observed and why?

### What ICMP packets are observed and why?

### What are the IP and MAC addresses associated to ICMP packets and why?

