## Virtual Switch for VPNs endpoints

This project implements a virtual switch (vSwitch) for VPN endpoints. The vSwitch facilitates communication between virtual network interfaces (TAP devices) and manages the forwarding of Ethernet frames between these interfaces.

+ `vPort`: client of the vSwitch, represents a virtual interface (TAP device) connected to the vSwitch It's responsible for sending and receiving Ethernet frames to and from the vSwitch.

+ `vSwitch`: manages the forwarding of Ethernet frames between vPorts. It stores MAC addresses of vPorts and forwards frames based on the destination MAC address.
<br> <br> 


Set up TAP devices and compile the project: 
```
sudo ./setup.sh
compile.sh
```
<br>


Run the server:
```
vswitch <port>
```
<br>


Run the client
```
vport <serverIp> <serverPort>
```
<br>


Clear binaries: 
```
cleanup.sh 
```

### Ethernet Frame Format
The Ethernet frame format used in this project is as follows:
```
+-----------------------------------------------+
|             Destination MAC Address           |   (6 bytes)
+-----------------------------------------------+
|             Source MAC Address                |   (6 bytes)
+-----------------------------------------------+
|             EtherType / Length                |   (2 bytes)
+-----------------------------------------------|
|                                               |
|                Payload / Data                 |   (46–1500 bytes)
|                                               |
+-----------------------------------------------+
|             Frame Check Sequence (FCS)        |   (4 bytes)
+-----------------------------------------------+
```