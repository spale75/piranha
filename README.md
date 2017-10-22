# Piranha

![alt text](https://github.com/spale75/piranha/raw/master/web/piranha_s.png "Piranha Logo")

### A highly efficient, multi-threaded, BGP route collector written in C.

Piranha collects routes and dumps them into files for further processing mainly for the purpose of analysis.
Piranha is NOT a BGP router and does NOT have the capability to announce routes nor does it interact with the kernel routing table.

Piranha supports:
* BGP capabilities negociation.
* BGP 4 octets ASN and AS_PATHs.
* TCP MD5 BGP session protection.
* IPv6 routes over IPv6 sockets.
* IPv4 routes over IPv4 sockets.

## Installation

#### Download
Download the latest version at https://github.com/spale75/piranha/archive/piranha-1.1.0.tar.gz
    user@piranha$ wget https://github.com/spale75/piranha/archive/piranha-1.1.0.tar.gz
    
####  Unpack
Unpack the TGZ file. Don't worry, all files contained in the TGZ are in a subfolder.
    
    user@piranha$ tar -zxvf piranha-1.1.0.tar.gz

#### Compile and Install

    user@piranha$ [sudo] ./compile.sh <destination folder>
    user@piranha$ [sudo] ./compile.sh /opt/piranha/

*NOTE: Might need sudo if your destination is not writable by the user.*

#### Debug
If you have troubles compiling the code, you can enable debug for compiling.

    user@piranha$ [sudo] ./compile.sh /opt/piranha debug

*NOTE: This will also enable DEBUG in the code. Piranha will output A LOT of debugging messages. Do NOT run this on production. Connect only one neighbor, the debugging is not designed to understand the messages when multiple peers are connected.*

#### Testing
In the source there is a "test" folder containing a shell script named "test.sh". Run this from the test folder and it will compare the decoding of sample dump files compared to a reference. Typical output:

	user@piranha$ ./test.sh
    Testing ipv4 in mode H: OK
    Testing ipv4 in mode m: OK
    Testing ipv4 in mode j: OK
    Testing ipv6 in mode H: OK
    Testing ipv6 in mode m: OK
    Testing ipv6 in mode j: OK
	user@piranha$


## Configuration
Piranha has one configuration file located in <destination folder>/etc/piranha.conf. In the same folder there is a sample configuration name piranha_sample.conf. Copy the file to piranha.conf and edit it.

### piranha.conf
```
# your local AS number
local_as <ASN>

# Listening IP/Port for IPv4 peers. If omitted, piranha will not listen for IPv4 connections.
local_ip4 <local IPv4>
local_port4 <tcp port>

# Listening IP/Port for IPv6 peers. If omitted, piranha will not listen for IPv6 connections.
local_ip6 <local IPv6>
local_port6 <tcp port>

# Export options: Choose which route attributes will be exported to the dump files
export origin       # IGP/EGP/Unknown
export aspath       # AS_PATH
export community    # COMMUNITY
export extcommunity # EXTENDED COMMUNITY

# BGP Router Identifier. This MUST be set and may not be 0.0.0.0.
# If you don't know what to put in this option, just copy your public IPv4
# address.
bgp_router_id <ipv4>

# The user that piranha will run as. Because piranha needs
# tcp port 179, it must be started as root. Piranha will then
# operator a privilege downgrade to this use for obvious security
# reasons.
user nobody

# Finally you must configure your BGP neighbors
# You may configure up to 128 neighbors
# (this can be changed in inc/p_defs.h:#define MAX_PEERS 128)
# The password is optional and is implemented as defined in RFC5425
neighbor <IPv4 or IPv6 address> <asn> [password]
```
## Usage
### Start/Stop/Restart
    <install dir>/etc/piranhactl <start|restart|stop>
### Status (state of all neighbors)
    cat <install dir>/var/piranha.status
### MAN Pages
    man -M <install dir>/man <ptoa|piranha|piranhactl|piranha.conf>

## Reading Piranha DUMP
Piranha dumps the received BGP Updates into dump files located in <install dir>/dump/<neighbor IP>. Files are rotated by default every 60 seconds. If there was no BGP message during that time, the dump not created for performance reasons. The 60 seconds interval can be tuned prior to compilation in `inc/p_defs.h:#define DUMPINTERVAL 60`.
Dump files ready to be read have the following format: `YYYYMMddhhmmss`.
With the tool `<install dir>/bin/ptoa` data from the dump files can be exported in three different formats:

* `./ptoa -H <dump file>`: Human readable format
* `./ptoa -m <dump file>`: Machine readable format
* `./ptoa -j <dump file>`: JSON format

### Examples
#### Human readable format
```
2017-10-21 21:31:54 peer ip 2a03:2260::5 AS 201701
2017-10-21 21:31:54 prefix announce 2a06:dac0::/29 origin IGP aspath 201701 13030 25180 202939 community 5093:5349 6629:6885 7141:7397
2017-10-21 21:31:55 eof
```
#### Machine readable format
```
1508621514|P|2a03:2260::5|201701
1508621514|A|2a06:dac0::|29|O|I|AP|201701 13030 25180 202939|C|5093:5349 5605:5861 6629:6885 7141:7397
1508621515|E
```
#### JSON format
```
{ "timestamp": 1508621514, "type": "peer", "msg": { "peer": { "proto": "ipv6", "ip": "2a03:2260::5", "asn": 201701 } } }
{ "timestamp": 1508621514, "type": "announce", "msg": { "prefix": "2a06:dac0::/29", "origin": "IGP", "aspath": [ 201701, 13030, 25180, 202939 ], "community": [ "5093:5349", "5605:5861", "6629:6885", "7141:7397" ] } }
{ "timestamp": 1508621515, "type": "footer" }
```

### Message type tags in DUMPs
Colons can be used to align columns.

| Human     | Machine | JSON | Description |
|:----------|:--------|:----------|:---------------------------------------------------------------------------------------------------------------|
| peer      | P       | peer      | First message in any dump describing the neighbor                                                              |
| announce  | A       | announce  | BGP prefix announce, optional origin (O), aspath (AP), community (C) and extended community (EC) subcomponents |
| withdrawn | W       | withdrawn | BGP prefix withdrawn                                                                                           |
| eof       | E       | footer    | Last message in any dump, has no other value                                                                   |


## Conformity
Piranha implements partially or completely the following RFCs:
* RFC1997: BGP Communities Attribute
* RFC4360: BGP Extended Communities Attribute
* RFC4760: Multiprotocol Extensions for BGP-4
* RFC4271: A Border Gateway Protocol 4 (BGP-4)
* RFC5425: The TCP Authentication Option
* RFC5492: Capabilities Advertisement with BGP-4
* RFC6793: BGP Support for Four-Octet Autonomous System (AS) Number Space

## Limitations
* Config reload does not work and may lead to a crash.
* Extended communities are not yet supported.
* Piranha is not able to communicate with BGP speakers not conforming to RFC5492 (old speakers).
* MD5 protection is only supported on Linux Kernels.

## Copyright

*Copyright 2004-2017 Pascal Gloor*
*Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0*
*Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.*


