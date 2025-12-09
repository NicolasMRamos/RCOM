# Experience 5 - DNS

### Step 1

To add DNS to each tux, we need to edit the file "/etc/resolv.conf":

```bash
sudo vim /etc/resolv.conf
```

Then, we add the following line:

```bash
nameserver 10.227.120.3
```

### Step 2

To test if DNS is working, we can ping different services:

```bash
ping services.netlab.fe.up.pt # ping feup netlab

ping www.google.com # ping google
```

### Step 3

Wireshark will show the DNS query and response.

The Wireshark log is available in "wslogs/exp5/exp5_log.pcapng".

## Questions

### How to configure the DNS service in a host?

### What packets are exchanged by DNS and what information is transported?