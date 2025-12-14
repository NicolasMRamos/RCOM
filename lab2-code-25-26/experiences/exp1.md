# Experience 1 - Configure an IP Network

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

## Questions

### What are the ARP packets and what are they used for?

### What are the MAC and IP addresses of ARP packets and why?

### What packets does the ping command generate?

### What are the MAC and IP addresses of the ping packets?

### How to determine if a receiving Ethernet frame is ARP, IP, ICMP?

### How to determine the length of a receiving frame?

### What is the loopback interface and why is it important?

