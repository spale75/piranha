/*******************************************************************************/
/*                                                                             */
/*  Copyright 2004-2017 Pascal Gloor                                                */
/*                                                                             */
/*  Licensed under the Apache License, Version 2.0 (the "License");            */
/*  you may not use this file except in compliance with the License.           */
/*  You may obtain a copy of the License at                                    */
/*                                                                             */
/*     http://www.apache.org/licenses/LICENSE-2.0                              */
/*                                                                             */
/*  Unless required by applicable law or agreed to in writing, software        */
/*  distributed under the License is distributed on an "AS IS" BASIS,          */
/*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   */
/*  See the License for the specific language governing permissions and        */
/*  limitations under the License.                                             */
/*                                                                             */
/*******************************************************************************/

#include <limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <p_endian.h>

#ifndef PIRANHA_DEFS
#define PIRANHA_DEFS

#include <inttypes.h>
#include <pwd.h>
#include <netinet/tcp.h>

#define P_VER_MA "1"
#define P_VER_MI "1"
#define P_VER_PL "1"

#define MAX_PEERS 128
#define DUMPINTERVAL 60

#if defined(OS_LINUX)
#define MAX_KEY_LEN (TCP_MD5SIG_MAXKEYLEN+1)
#else
#define MAX_KEY_LEN 1
#endif

#ifndef PATH
#define PATH "./"
#endif

#define CHOMP(x) if ( x[strlen(x)-2]=='\r' ) { x[strlen(x)-2] = '\0'; } else if ( x[strlen(x)-1]=='\n' ) { x[strlen(x)-1] = '\0'; }

#if !defined(s6_addr32)
#  if defined(FREEBSD) || defined(NETBSD) || defined(OPENBSD) || defined(DARWIN)|| defined(DRAGONFLY)
#    define s6_addr32   __u6_addr.__u6_addr32
#  endif
#endif

#define LOGFILE    PATH "/var/piranha.log"
#define STATUSFILE PATH "/var/piranha.status"
#define STATUSTEMP PATH "/var/piranha.status.temp"
#define PIDFILE    PATH "/var/piranha.pid"
#define DUMPDIR    PATH "/var/dump"

#define INPUT_BUFFER  1048576
#define OUTPUT_BUFFER 65536
#define TEMP_BUFFER   65536

#define BGP_HEADER_LEN  19
#define BGP_OPEN_LEN    10
#define BGP_ERROR_LEN   8

#define BGP_DEFAULT_HOLD 180

#define BGP_OPEN	1
#define BGP_UPDATE	2
#define BGP_ERROR	3
#define BGP_KEEPALIVE	4

#define BGP_ATTR_ORIGIN			1
#define BGP_ATTR_AS_PATH		2
#define BGP_ATTR_NEXT_HOP		3
#define BGP_ATTR_COMMUNITY		8
#define BGP_ATTR_MP_REACH_NLRI		14
#define BGP_ATTR_MP_UNREACH_NLRI	15
#define BGP_ATTR_EXTCOMMUNITY		16

#define EXPORT_ORIGIN       0x01
#define EXPORT_ASPATH       0x02
#define EXPORT_COMMUNITY    0x04
#define EXPORT_EXTCOMMUNITY 0x08

#define DUMP_OPEN        10
#define DUMP_CLOSE       11
#define DUMP_KEEPALIVE   12

#define DUMP_HEADER4     40
#define DUMP_ANNOUNCE4   41
#define DUMP_WITHDRAWN4  42

#define DUMP_HEADER6     60 
#define DUMP_ANNOUNCE6   61 
#define DUMP_WITHDRAWN6  62

#define DUMP_FOOTER     255

#ifdef CC_SUNCC
#pragma packed()
#endif

struct bgp_header
{
	char     marker[16];
	uint16_t len;
	uint8_t  type;
#ifdef CC_GCC
} __attribute__((packed));
#else
};
#endif

struct bgp_open
{
	uint8_t  version;
	uint16_t as;
	uint16_t holdtime;
	uint32_t bgp_id;
	uint8_t  param_len;
#ifdef CC_GCC
} __attribute__((packed));
#else
};
#endif

struct bgp_param
{
	uint8_t type;
	uint8_t len;
	char    param[256];
#ifdef CC_GCC
} __attribute__((packed));
#else
};
#endif

struct bgp_param_capa
{
	uint8_t type;
	uint8_t len;
	union {
		char def[256];
		uint32_t as4;
	} u;
#ifdef CC_GCC
} __attribute__((packed));
#else
};
#endif

struct bgp_error
{
	uint8_t code;
	uint8_t subcode;
	char    data[6];
#ifdef CC_GCC
} __attribute__((packed));
#else
};
#endif

struct dump_msg
{
	uint8_t type;
	uint16_t len;
	uint64_t ts;
#ifdef CC_GCC
} __attribute__((packed));
#else
};
#endif

struct dump_header4
{
	uint32_t ip;
	uint32_t as;
#ifdef CC_GCC
} __attribute__((packed));
#else
};
#endif

struct dump_header6
{
	uint8_t ip[16];
	uint32_t as;
#ifdef CC_GCC
} __attribute__((packed));
#else
};
#endif

struct dump_withdrawn4
{
	uint8_t  mask;
	uint32_t prefix;
#ifdef CC_GCC
} __attribute__((packed));
#else
};
#endif

struct dump_withdrawn6
{
	uint8_t mask;
	uint8_t prefix[16];
#ifdef CC_GCC
} __attribute__((packed));
#else
};
#endif

struct dump_announce4
{
	uint8_t  mask;
	uint32_t prefix;
	uint8_t  origin;
	uint8_t  aspathlen;
	uint8_t  communitylen;
	uint8_t  extcommunitylen;
#ifdef CC_GCC
} __attribute__((packed));
#else
};
#endif

struct dump_announce_aspath
{
	uint32_t data[256];
#ifdef CC_GCC
} __attribute__((packed));
#else
};
#endif

struct dump_announce6
{
	uint8_t mask;
	uint8_t prefix[16];
	uint8_t origin;
	uint8_t aspathlen;
	uint8_t communitylen;
	uint8_t extcommunitylen;
#ifdef CC_GCC
} __attribute__((packed));
#else
};
#endif

struct dump_announce_community
{
	struct {
		uint16_t asn;
		uint16_t num;
	} data[256];
#ifdef CC_GCC
} __attribute__((packed));
#else
};
#endif

struct dump_announce_extcommunity
{
	struct {
		uint32_t ip;
		uint32_t num;
	} data[256];
#ifdef CC_GCC
} __attribute__((packed));
#else
};
#endif

struct dump_full_msg
{
	struct dump_msg msg;
	union {
		struct dump_header4 header4;
		struct dump_header6 header6;
		struct dump_announce4 announce4;
		struct dump_announce6 announce6;
		struct dump_withdrawn4 withdrawn4;
		struct dump_withdrawn6 withdrawn6;
	};
	struct dump_announce_aspath aspath;
	struct dump_announce_community community;
	struct dump_announce_extcommunity extcommunity;
#ifdef CC_GCC
} __attribute__((packed));
#else
};
#endif

struct dump_file_ctx
{
	char file[PATH_MAX];
	FILE *fh;
	int head;
	int end;
	int pos;
};




struct config_t
{
	struct {
		struct sockaddr_in  listen;
		int sock;
		int enabled;
	} ip4;
	struct {
		struct sockaddr_in6 listen;
		int sock;
		int enabled;
	} ip6;
	uint8_t export;
	uint32_t as;
	uint32_t routerid;
	uint16_t holdtime;
	uid_t    uid;
	gid_t    gid;
	char     *file;
	struct peer_t *peer;
};

struct peer_t
{
	uint8_t  allow;
	uint8_t  newallow;         /* to avoid peer drop during reconfiguration */
	uint8_t  status;           /* 0 offline, 1 connected, 2 authed */
	uint32_t ucount;           /* bgp updates count */
	union {
		struct   in6_addr ip6;     /* peer IPv4 address */
		struct   in_addr  ip4;     /* peer IPv6 address */
	};
	uint8_t  af;               /* indicates wether peer is v4 or v6 */
	uint32_t as;               /* ASN */
	char     key[MAX_KEY_LEN]; /* MD5 authentication, null terminated */
	uint32_t rmsg;
	uint32_t smsg;
	uint64_t cts;
	uint64_t rts;
	uint64_t sts;
	uint16_t rhold;
	uint16_t shold;
	uint8_t  as4;              /* neighbor 4 bytes AS advertised capability support. */
	FILE     *fh;
	uint8_t  empty;
	char     filename[1024];
	uint64_t filets;
	int      ilen;
	int      olen;
	int      sock;
};

#endif
