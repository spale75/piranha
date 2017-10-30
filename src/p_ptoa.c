/*******************************************************************************/
/*                                                                             */
/*  Copyright 2004-2017 Pascal Gloor                                           */
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
#include <stdlib.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>


#include <p_defs.h>
#include <p_ptoa.h>
#include <p_dump.h>
#include <p_undump.h>
#include <p_tools.h>

/* dump decoder tool */
int main(int argc, char *argv[])
{
	char *file = NULL;
	int mode = PTOA_NONE;
	struct dump_file_ctx *ctx;

	if ( argc != 3 )
		syntax(argv[0]);
	else if ( strcmp(argv[1],"-m") == 0 )
		mode = PTOA_MACHINE;
	else if ( strcmp(argv[1],"-H") == 0 )
		mode = PTOA_HUMAN;
	else if ( strcmp(argv[1],"-j") == 0 )
		mode = PTOA_JSON;
	else
		syntax(argv[0]);

	file = argv[2];

	if ( ( ctx = p_undump_open(file) ) == NULL )
	{
		fprintf(stderr,"error opening '%s'\n",file);
		return -1;
	}

	while(!ctx->end)
	{
		struct dump_full_msg msg;

		if ( p_undump_readmsg(ctx, &msg) != 0 )
		{
			fprintf(stderr,"error during message parsing of '%s'\n", ctx->file);
			break;
		}

		switch(mode) {
			case PTOA_MACHINE:
				{
					unsigned long long int ts  = msg.msg.ts;
					unsigned long long int uts = msg.msg.uts;
					printf("%llu.%llu|",ts,uts);
				}
				break;
			case PTOA_HUMAN:
				{
					char line[100];
					struct timeval t;
					t.tv_sec  = msg.msg.ts;
					t.tv_usec = msg.msg.uts;

					p_tools_humantime(line, sizeof(line), &t);
					printf("%s ",line);
				}
				break;
			case PTOA_JSON:
				{
					unsigned long long int ts  = msg.msg.ts;
					unsigned long long int uts = msg.msg.uts;
					printf("{ \"timestamp\": %llu.%llu, ",ts, uts);
				}
		}

		switch(msg.msg.type)
		{
			case DUMP_HEADER4:
				if ( mode == PTOA_MACHINE )
					printf("P|%u|%u|%c\n",msg.header4.ip,msg.header4.as,msg.header4.type == BGP_TYPE_IBGP ? 'i' : 'e');
				else if ( mode == PTOA_JSON )
				{
					struct in_addr addr;
					addr.s_addr = htobe32(msg.header4.ip);
					printf("\"type\": \"peer\", \"msg\": { \"peer\": { \"proto\": \"ipv4\", \"ip\": \"%s\", \"asn\": %u, \"type\": %s } } }\n",
						inet_ntoa(addr), msg.header4.as, msg.header4.type == BGP_TYPE_IBGP ? "ibgp" : "ebgp" );
				}
				else
				{
					struct in_addr addr;
					addr.s_addr = htobe32(msg.header4.ip);
					printf("peer ip %s AS %u TYPE %s\n",inet_ntoa(addr),msg.header4.as,msg.header4.type == BGP_TYPE_IBGP ? "ibgp" : "ebgp");
				}
				break;

			case DUMP_HEADER6:
				if ( mode == PTOA_MACHINE )
				{
					struct in6_addr addr;
					memcpy(addr.s6_addr, msg.header6.ip, sizeof(msg.header6.ip));
					printf("P|%s|%u|%c\n",p_tools_ip6str(MAX_PEERS, &addr),msg.header6.as,msg.header6.type == BGP_TYPE_IBGP ? 'i' : 'e');
				}
				else if ( mode == PTOA_JSON )
				{
					struct in6_addr addr;
					memcpy(addr.s6_addr, msg.header6.ip, sizeof(msg.header6.ip));
					printf("\"type\": \"peer\", \"msg\": { \"peer\": { \"proto\": \"ipv6\", \"ip\": \"%s\", \"asn\": %u, \"type\": %s } } }\n",
						p_tools_ip6str(MAX_PEERS, &addr),msg.header6.as,msg.header6.type == BGP_TYPE_IBGP ? "ibgp" : "ebgp");
				}
				else
				{
					struct in6_addr addr;
					memcpy(addr.s6_addr, msg.header6.ip, sizeof(msg.header6.ip));
					printf("peer ip %s AS %u TYPE %s\n",p_tools_ip6str(MAX_PEERS, &addr),msg.header6.as,msg.header6.type == BGP_TYPE_IBGP ? "ibgp" : "ebgp");
				}
				break;

			case DUMP_OPEN:
				if ( mode == PTOA_MACHINE )
					printf("C\n");
				else if ( mode == PTOA_JSON )
					printf("\"type\": \"connect\" }\n");
				else
					printf("connected\n");
				break;

			case DUMP_CLOSE:
				if ( mode == PTOA_MACHINE )
					printf("D\n");
				else if ( mode == PTOA_JSON )
					printf("\"type\": \"disconnect\" }\n");
				else
					printf("disconnected\n");
				break;

			case DUMP_KEEPALIVE:
				if ( mode == PTOA_MACHINE )
					printf("K\n");
				else if ( mode == PTOA_JSON )
					printf("\"type\": \"keepalive\" }\n");
				else
					printf("keepalive\n");
				break;

			case DUMP_ANNOUNCE4:

				if ( mode == PTOA_MACHINE )
					printf("A|%u|%u",msg.announce4.prefix, msg.announce4.mask);
				else if ( mode == PTOA_JSON )
				{
					struct in_addr addr;
					addr.s_addr = htobe32(msg.announce4.prefix);
					printf("\"type\": \"announce\", \"msg\": { \"prefix\": \"%s/%u\"",
						inet_ntoa(addr),msg.announce4.mask);
				}
				else
				{
					struct in_addr addr;
					addr.s_addr = htobe32(msg.announce4.prefix);
					printf("prefix announce %s/%u",inet_ntoa(addr),msg.announce4.mask);
				}

				if ( msg.announce4.origin != 0xff )
					print_origin(mode, msg.announce4.origin);

				if ( msg.announce4.aspathlen > 0 )
					print_aspath(mode, &msg.aspath, msg.announce4.aspathlen);

				if ( msg.announce4.communitylen > 0 )
					print_community(mode, &msg.community, msg.announce4.communitylen);

				if ( msg.announce4.extcommunitylen4 > 0 )
					print_extcommunity4(mode, &msg.extcommunity4, msg.announce4.extcommunitylen4);

				if ( msg.announce4.largecommunitylen > 0 )
					print_largecommunity(mode, &msg.largecommunity, msg.announce4.largecommunitylen);


				if ( mode == PTOA_JSON )
					printf(" } }\n");
				else
					printf("\n");

				break;

			case DUMP_ANNOUNCE6:
				if ( mode == PTOA_MACHINE )
				{
					struct in6_addr addr;
					memcpy(addr.s6_addr, msg.announce6.prefix, sizeof(msg.announce6.prefix));
					printf("A|%s|%u",p_tools_ip6str(0, &addr), msg.announce6.mask);
				}
				else if ( mode == PTOA_JSON )
				{
					struct in6_addr addr;
					memcpy(addr.s6_addr, msg.announce6.prefix, sizeof(msg.announce6.prefix));
					printf("\"type\": \"announce\", \"msg\": { \"prefix\": \"%s/%u\"",
						p_tools_ip6str(0, &addr),msg.announce6.mask);
				}
				else
				{
					struct in6_addr addr;
					memcpy(addr.s6_addr, msg.announce6.prefix, sizeof(msg.announce6.prefix));
					printf("prefix announce %s/%u",p_tools_ip6str(0, &addr),msg.announce6.mask);
				}

				if ( msg.announce6.origin != 0xff )
					print_origin(mode, msg.announce6.origin);

				if ( msg.announce6.aspathlen > 0 )
					print_aspath(mode, &msg.aspath, msg.announce6.aspathlen);

				if ( msg.announce6.communitylen > 0 )
					print_community(mode, &msg.community, msg.announce6.communitylen);

				if ( msg.announce6.extcommunitylen6 > 0 )
					print_extcommunity6(mode, &msg.extcommunity6, msg.announce6.extcommunitylen6);

				if ( msg.announce6.largecommunitylen > 0 )
					print_largecommunity(mode, &msg.largecommunity, msg.announce6.largecommunitylen);

				if ( mode == PTOA_JSON )
					printf(" } }");
	
				printf("\n");

				break;

			case DUMP_WITHDRAWN4:

				if ( mode == PTOA_MACHINE )
				{
					printf("W|%u|%u",msg.withdrawn4.prefix,msg.withdrawn4.mask);
				}
				else if ( mode == PTOA_JSON )
				{
					struct in_addr addr;
					addr.s_addr = htobe32(msg.withdrawn4.prefix);
					printf("\"type\": \"withdrawn\", \"msg\": { \"prefix\": \"%s/%u\" } }",
						inet_ntoa(addr),msg.withdrawn4.mask);
				}
				else
				{
					struct in_addr addr;
					addr.s_addr = htobe32(msg.withdrawn4.prefix);
					printf("prefix withdrawn %s/%u",inet_ntoa(addr),msg.withdrawn4.mask);
				}
				printf("\n");

				break;

			case DUMP_WITHDRAWN6:
				if ( mode == PTOA_MACHINE )
				{
					struct in6_addr addr;
					memcpy(addr.s6_addr, msg.withdrawn6.prefix, sizeof(msg.withdrawn6.prefix));
					printf("W|%s|%u",p_tools_ip6str(0, &addr), msg.withdrawn6.mask);
				}
				else if ( mode == PTOA_JSON )
				{
					struct in6_addr addr;
					memcpy(addr.s6_addr, msg.withdrawn6.prefix, sizeof(msg.withdrawn6.prefix));
					printf("\"type\": \"withdrawn\", \"msg\": { \"prefix\": \"%s/%u\" } }",
						p_tools_ip6str(0, &addr),msg.withdrawn6.mask);
				}
				else
				{
					struct in6_addr addr;
					memcpy(addr.s6_addr, msg.withdrawn6.prefix, sizeof(msg.withdrawn6.prefix));
					printf("prefix withdrawn %s/%u",p_tools_ip6str(0, &addr),msg.withdrawn6.mask);
				}
				printf("\n");

				break;

			case DUMP_FOOTER:

				if ( mode == PTOA_MACHINE )
					printf("E\n");
				else if ( mode == PTOA_JSON )
					printf("\"type\": \"footer\" }\n");
				else
					printf("eof\n");

				break;

			default:
				fprintf(stderr, "Error: received unknown message code: %u\n", msg.msg.type);
		}
	}

	p_undump_close(ctx);

	return 0;
}

/* syntax */
void syntax(char *prog)
{
	printf("Piranha v%s.%s.%s Dump file decoder, Copyright(c) 2004-2017 Pascal Gloor\n",P_VER_MA,P_VER_MI,P_VER_PL);
	printf("syntax: %s -<m|j|H> <file>\n",prog);
	printf("\n");
	printf("-H for human readable output\n");
	printf("\n");
	printf("-j for JSON output\n");
	printf("One JSON elem per line.\n");
	printf("\n");
	printf("-m for machine readable output:\n");
	printf("timestamp|P|peer_ip|peer_as # begin of every file\n");
	printf("timestamp|C                 # connected (Active -> Established)\n");
	printf("timestamp|D                 # disconnected (Established -> Active)\n");
	printf("timestamp|K                 # BGP Keepalive received\n");
	printf("timestamp|A|network|mask|opt id|opt|opt id ...\n");
	printf("                            # BGP Announce\n");
	printf("timestamp|W|network|mask    # BGP Withdrawn\n");
	printf("\n");
	
	exit(-1);
}

void print_origin(int mode, uint8_t origin)
{
	char o = '?';
	char oa[10];

	switch(origin)
	{
		case BGP_ORIGIN_IGP:
			o = 'I';
			snprintf(oa, sizeof(oa), "IGP");
			break;
		case BGP_ORIGIN_EGP:
			o = 'E';
			snprintf(oa, sizeof(oa), "EGP");
			break;
		case BGP_ORIGIN_UNKN:
			o = '?';
			snprintf(oa, sizeof(oa), "Unknown");
			break;
		default:
			o = '?';
			snprintf(oa, sizeof(oa), "Error");
			break;
	}

	switch(mode) {
		case PTOA_MACHINE:
			printf("|O|%c", o);
			break;
		case PTOA_HUMAN:
			printf(" origin %s", oa);
			break;
		case PTOA_JSON:
			printf(", \"origin\": \"%s\"", oa);
			break;
	}
}


void print_aspath(int mode, struct dump_announce_aspath *aspath, uint8_t len)
{
	int i;

	switch(mode) {
		case PTOA_MACHINE: printf("|AP|"); break;
		case PTOA_HUMAN: printf(" aspath"); break;
		case PTOA_JSON: printf(", \"aspath\": [ "); break;
	}

	for(i=0; i<len; i++)
	{
		switch(mode) {
			case PTOA_MACHINE:
				printf("%u", aspath->data[i]);
				if ( i < len-1 ) printf(" ");
				break;
			case PTOA_HUMAN:
				printf(" %u", aspath->data[i]);
				break;
			case PTOA_JSON:
				printf("%u", aspath->data[i]);
				if ( i < len-1 ) printf(", ");
				break;
		}
	}

	if ( mode == PTOA_JSON )
		printf(" ]");
}

void print_community(int mode, struct dump_announce_community *community, uint16_t len)
{
	int i;

	if ( mode == PTOA_MACHINE )
		printf("|C|");
	else if ( mode == PTOA_JSON )
		printf(", \"community\": [ ");
	else
		printf(" community");

	for(i=0; i<len; i++)
	{
		if ( mode == PTOA_MACHINE )
		{
			printf("%u:%u",community->data[i].asn, community->data[i].num);
			if ( i < len-1) printf(" ");
		}
		else if ( mode == PTOA_JSON )
		{
			printf("\"%u:%u\"",community->data[i].asn, community->data[i].num);
			if ( i < len-1) printf(", ");
		}
		else
		{
			printf(" %u:%u", community->data[i].asn, community->data[i].num);
		}
	}

	if ( mode == PTOA_JSON )
		printf(" ]");
}

void print_extcommunity4(int mode, struct dump_announce_extcommunity4 *com, uint16_t len)
{
	/* not yet implemented */
}

void print_extcommunity6(int mode, struct dump_announce_extcommunity6 *com, uint16_t len)
{
	/* not yet implemented */
}

void print_largecommunity(int mode, struct dump_announce_largecommunity *community, uint16_t len)
{
	int i;

	if ( mode == PTOA_MACHINE )
		printf("|LC|");
	else if ( mode == PTOA_JSON )
		printf(", \"largecommunity\": [ ");
	else
		printf(" largecommunity");

	for(i=0; i<len; i++)
	{
		if ( mode == PTOA_MACHINE )
		{
			printf("%u:%u:%u",community->data[i].global, community->data[i].local1, community->data[i].local2);
			if ( i < len-1) printf(" ");
		}
		else if ( mode == PTOA_JSON )
		{
			printf("\"%u:%u:%u\"",community->data[i].global, community->data[i].local1, community->data[i].local2);
			if ( i < len-1) printf(", ");
		}
		else
		{
			printf(" %u:%u:%u", community->data[i].global, community->data[i].local1, community->data[i].local2);
		}
	}

	if ( mode == PTOA_JSON )
		printf(" ]");
}

