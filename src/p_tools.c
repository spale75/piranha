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

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/param.h>
#include <fcntl.h>
#include <sys/time.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>
#include <assert.h>

#include <p_defs.h>
#include <p_tools.h>

int p_tools_ip4zero(struct in_addr *ip)
{
	if ( ip != NULL && ip->s_addr == 0 )
		return 1;

	return 0;
}
int p_tools_ip6zero(struct in6_addr *ip)
{
	if ( ip != NULL &&
	     ip->s6_addr32[0] == 0 &&
	     ip->s6_addr32[1] == 0 &&
	     ip->s6_addr32[2] == 0 &&
	     ip->s6_addr32[3] == 0 )
		return 1;

	return 0;
}

int p_tools_sameip4(struct in_addr *ip1, struct in_addr *ip2)
{
	if ( ip1->s_addr == ip2->s_addr )
		return 1;

	return 0;
}

int p_tools_sameip6(struct in6_addr *ip1, struct in6_addr *ip2)
{
	if ( ip1->s6_addr32[0] == ip2->s6_addr32[0] &&
	     ip1->s6_addr32[1] == ip2->s6_addr32[1] &&
	     ip1->s6_addr32[2] == ip2->s6_addr32[2] &&
	     ip1->s6_addr32[3] == ip2->s6_addr32[3] )
		return 1;

	return 0;
}

char *p_tools_ip4str(int peerid, struct in_addr *ip)
{
	static char buf[MAX_PEERS+1][INET_ADDRSTRLEN];

	assert(peerid <= MAX_PEERS ? 1 : 0);

	inet_ntop(AF_INET, ip, buf[peerid], sizeof(buf[peerid]));

	return buf[peerid];
}

char *p_tools_ip6str(int peerid, struct in6_addr *ip)
{
	static char buf[MAX_PEERS+1][INET6_ADDRSTRLEN];

	assert(peerid <= MAX_PEERS ? 1 : 0);

	inet_ntop(AF_INET6, ip, buf[peerid], sizeof(buf[peerid]));

	return buf[peerid];
}

void p_tools_dump(const char *desc, char *data, int len)
{
	int full8 = len / 8 + 1;
	int i,j;

	printf("DUMP BEGIN (length: %i octets)\n", len);
	printf("-- %s\n", desc);

	for(i=0; i<full8; i++)
	{
		printf("0x%02x: ", i*8);
		for(j=0; j<8; j++)
			if ( i*8 + j < len )
				printf("%3u ", *(uint8_t *)(data + (i*8) + j));
			else
				printf("    ");

		for(j=0; j<8; j++)
			if ( i*8 + j < len )
				printf("%02x ", *(uint8_t *)(data + (i*8) + j));
			else
				printf("   ");

		printf("\n");
	}
	printf("DUMP END\n");
}

void p_tools_humantime(char *line, size_t len, struct timeval *ts)
{
	struct tm *tm;
	tm = gmtime(&ts->tv_sec);
	strftime(line, len, "%Y-%m-%d %H:%M:%S", tm);
	snprintf(line + strlen(line), len - strlen(line), ".%03u", ts->tv_usec / 1000);
}
