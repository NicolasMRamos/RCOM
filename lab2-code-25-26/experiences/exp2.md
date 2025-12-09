# Experience 2 - Implement two bridges in a switch

### Step 1

E1 of tux122 was connected to ether19 in the switch. It's MAC and IP address are as follows:

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

The Wireshark log is available in "wslogs/exp2/exp2_log2.pcapng".

### Step 10

Steps 7, 8 and 9 are repeated, but for tux122.

The Wireshark log is available in "wslogs/exp2/exp2_log3.pcapng".

## Questions

### How to configure bridgeY0?

### How many broadcast domains are there? How can you conclude it from the logs?
