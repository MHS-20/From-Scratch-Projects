## DNS Server Implementation

Implementation of a DNS server that adheres to the standard DNS protocol.
It parses zone files to load DNS records, receives DNS requests, and processes them to return matching domain names. The server handles all fields of the DNS format, including the header, question section, and resource record sections.

Usage:

```
node index.js
```

<br> Query the server using the example file:

```
dig www.example.com @127.0.0.1
```
<br/>

### Overall Message Format

```
    +---------------------+
    |        Header       |
    +---------------------+
    |       Question      |  the question for the name server
    +---------------------+
    |        Answer       |  RRs answering the question
    +---------------------+
    |      Authority      |  RRs pointing toward an authority
    +---------------------+
    |      Additional     |  RRs holding additional information
    +---------------------+
```
<br/>

### Header Fields

```
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                      ID                       |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    QDCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    ANCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    NSCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    ARCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
```
<br>

### Question Section

```
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                                               |
    |                     QNAME                     |
    |                                               |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                     QTYPE                     |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                     QCLASS                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

```
