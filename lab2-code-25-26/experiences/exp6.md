# Experience 6 - TCP connections

### Steps 1, 2 and 3

To compile the client:

``` bash
gcc clientFTP.c -o ftp
```

To test the client, we tried downloading the file "pipe.txt" from the ftp server:

``` bash
./ftp ftp://ftp.netlab.fe.up.pt/pipe.txt
```

The downloaded file is in "wslogs/exp6/pipe.txt"

The Wireshark log is available in "wslogs/exp6/ftptest.pcapng"

### Step 4

### Step 5

## Questions

### How many TCP connections are opened by your FTP application?
### In what connection is transported the FTP control information?
### What are the phases of a TCP connection?
### How does the ARQ TCP mechanism work? What are the relevant TCP fields?
### What relevant information can be observed in the logs?
### How does the TCP congestion control mechanism work? What are the relevant fields. How did the throughput of the data connection evolve along the time? Is it according to the TCP congestion control mechanism?
### Is the throughput of a TCP data connections disturbed by the appearance of a second TCP connection? How?