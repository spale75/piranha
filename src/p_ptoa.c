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
					unsigned long long int ts = msg.msg.ts;
					printf("%llu|",ts);
				}
				break;
			case PTOA_HUMAN:
				{
					char line[100];
					time_t ts = msg.msg.ts;

					p_tools_humantime(line, sizeof(line), ts);
					printf("%s ",line);
				}
				break;
			case PTOA_JSON:
				{
					unsigned long long int ts = msg.msg.ts;
					printf("{ \"timestamp\": %llu, ",ts);
				}
		}

		switch(msg.msg.type)
		{
			case DUMP_HEADER4:
				if ( mode == PTOA_MACHINE )
					printf("P|%u|%u\n",msg.header4.ip,msg.header4.as);
				else if ( mode == PTOA_JSON )
				{
					struct in_addr addr;
					addr.s_addr = htobe32(msg.header4.ip);
					printf("\"type\": \"peer\", \"msg\": { \"peer\": { \"proto\": \"ipv4\", \"ip\": \"%s\", \"asn\": %u } } }\n",
						inet_ntoa(addr), msg.header4.as);
				}
				else
				{
					struct in_addr addr;
					addr.s_addr = htobe32(msg.header4.ip);
					printf("peer ip %s AS %u\n",inet_ntoa(addr),msg.header4.as);
				}
				break;

			case DUMP_HEADER6:
				if ( mode == PTOA_MACHINE )
				{
					struct in6_addr addr;
					memcpy(addr.s6_addr, msg.header6.ip, sizeof(msg.header6.ip));
					printf("P|%s|%u\n",p_tools_ip6str(MAX_PEERS, &addr),msg.header6.as);
				}
				else if ( mode == PTOA_JSON )
				{
					struct in6_addr addr;
					memcpy(addr.s6_addr, msg.header6.ip, sizeof(msg.header6.ip));
					printf("\"type\": \"peer\", \"msg\": { \"peer\": { \"proto\": \"ipv6\", \"ip\": \"%s\", \"asn\": %u } } }\n",
						p_tools_ip6str(MAX_PEERS, &addr),msg.header6.as);
				}
				else
				{
					struct in6_addr addr;
					memcpy(addr.s6_addr, msg.header6.ip, sizeof(msg.header6.ip));
					printf("peer ip %s AS %u\n",p_tools_ip6str(MAX_PEERS, &addr),msg.header6.as);
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
				{
					if ( mode == PTOA_MACHINE )
					{
						char o = '?';
						switch(msg.announce4.origin)
						{
							case 0: o = 'I'; break;
							case 1: o = 'E'; break;
							case 2: o = '?'; break;
						}
						printf("|O|%c", o);
					}
					else
					{
						char o[10];
						switch(msg.announce4.origin)
						{
							case 0: sprintf(o, "IGP"); break;
							case 1: sprintf(o, "EGP"); break;
							case 2: sprintf(o, "Unknown"); break;
							default: sprintf(o, "Error"); break;
						}
						if ( mode == PTOA_JSON )
							printf(", \"origin\": \"%s\"", o);
						else
							printf(" origin %s", o);
					}
				}

				if ( msg.announce4.aspathlen > 0 )
				{
					int i;

					if ( mode == PTOA_MACHINE )
						printf("|AP|");
					else if ( mode == PTOA_JSON )
						printf(", \"aspath\": [ ");
					else
						printf(" aspath");

					for(i=0; i<msg.announce4.aspathlen; i++)
					{
						if ( mode == PTOA_MACHINE )
						{
							printf("%u", msg.aspath.data[i]);
							if ( i < msg.announce4.aspathlen-1 ) printf(" ");
						}
						else if ( mode == PTOA_JSON )
						{
							printf("%u", msg.aspath.data[i]);
							if ( i < msg.announce4.aspathlen-1 ) printf(", ");
						}
						else
						{
							printf(" %u", msg.aspath.data[i]);
						}
					}

					if ( mode == PTOA_JSON )
						printf(" ]");
	
				}

				if ( msg.announce4.communitylen > 0 )
				{
					int i;
	
					if ( mode == PTOA_MACHINE )
						printf("|C|");
					else if ( mode == PTOA_JSON )
						printf(", \"community\": [ ");
					else
						printf(" community");
	
					for(i=0; i<msg.announce4.communitylen; i++)
					{
						if ( mode == PTOA_MACHINE )
						{
							printf("%u:%u",msg.community.data[i].asn, msg.community.data[i].num);
							if ( i < msg.announce4.communitylen-1) printf(" ");
						}
						else if ( mode == PTOA_JSON )
						{
							printf("\"%u:%u\"",msg.community.data[i].asn, msg.community.data[i].num);
							if ( i < msg.announce4.communitylen-1) printf(", ");
						}
						else
						{
							printf(" %u:%u", msg.community.data[i].asn, msg.community.data[i].num);
						}
					}

					if ( mode == PTOA_JSON )
						printf(" ]");
				}

				if ( msg.announce4.extcommunitylen > 0 )
				{
					int i;
	
					if ( mode == PTOA_MACHINE )
						printf("|EC|");
					else if ( mode == PTOA_JSON )
						printf(", \"extcommunity\": [ ");
					else
						printf(" extcommunity");
	
					for(i=0; i<msg.announce4.extcommunitylen; i++)
					{
						if ( mode == PTOA_MACHINE )
						{
							printf("%u:%u",msg.extcommunity.data[i].ip, msg.extcommunity.data[i].num);
							if ( i < msg.announce4.extcommunitylen-1 ) printf(" ");
						}
						else if ( mode == PTOA_JSON )
						{
							printf("\"%u:%u\"",msg.extcommunity.data[i].ip, msg.extcommunity.data[i].num);
							if ( i < msg.announce4.extcommunitylen-1 ) printf(", ");
						}
						else
						{
							struct in_addr addr;
							addr.s_addr = msg.extcommunity.data[i].ip;
							printf(" %s:%u", inet_ntoa(addr), msg.extcommunity.data[i].num);
						}
					}

					if ( mode == PTOA_JSON )
						printf(" ]");
				}
	
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
				{
					if ( mode == PTOA_MACHINE )
					{
						char o = '?';
						switch(msg.announce6.origin)
						{
							case 0: o = 'I'; break;
							case 1: o = 'E'; break;
							case 2: o = '?'; break;
						}
						printf("|O|%c", o);
					}
					else
					{
						char o[10];
						switch(msg.announce6.origin)
						{
							case 0: sprintf(o, "IGP"); break;
							case 1: sprintf(o, "EGP"); break;
							case 2: sprintf(o, "Unknown"); break;
							default: sprintf(o, "Error"); break;
						}
						if ( mode == PTOA_JSON )
							printf(", \"origin\": \"%s\"", o);
						else
							printf(" origin %s", o);
					}
				}

				if ( msg.announce6.aspathlen > 0 )
				{
					int i;

					if ( mode == PTOA_MACHINE )
						printf("|AP|");
					else if ( mode == PTOA_JSON )
						printf(", \"aspath\": [ ");
					else
						printf(" aspath");

					for(i=0; i<msg.announce6.aspathlen; i++)
					{
						if ( mode == PTOA_MACHINE )
						{
							printf("%u", msg.aspath.data[i]);
							if ( i < msg.announce6.aspathlen-1 ) printf(" ");
						}
						else if ( mode == PTOA_JSON )
						{
							printf("%u", msg.aspath.data[i]);
							if ( i < msg.announce6.aspathlen-1 ) printf(", ");
						}
						else
						{
							printf(" %u", msg.aspath.data[i]);
						}
					}

					if ( mode == PTOA_JSON )
						printf(" ]");
	
				}

				if ( msg.announce6.communitylen > 0 )
				{
					int i;
	
					if ( mode == PTOA_MACHINE )
						printf("|C|");
					else if ( mode == PTOA_JSON )
						printf(", \"community\": [ ");
					else
						printf(" community");
	
					for(i=0; i<msg.announce6.communitylen; i++)
					{
						if ( mode == PTOA_MACHINE )
						{
							printf("%u:%u",msg.community.data[i].asn, msg.community.data[i].num);
							if ( i < msg.announce6.communitylen-1 ) printf(" ");
						}
						else if ( mode == PTOA_JSON )
						{
							printf("\"%u:%u\"",msg.community.data[i].asn, msg.community.data[i].num);
							if ( i < msg.announce6.communitylen-1 ) printf(", ");
						}
						else
						{
							printf(" %u:%u", msg.community.data[i].asn, msg.community.data[i].num);
						}
					}

					if ( mode == PTOA_JSON )
						printf(" ]");
				}

				if ( msg.announce6.extcommunitylen > 0 )
				{
					int i;
	
					if ( mode == PTOA_MACHINE )
						printf("|EC|");
					else if ( mode == PTOA_JSON )
						printf(", \"extcommunity\": [ ");
					else
						printf(" extcommunity");
	
					for(i=0; i<msg.announce6.extcommunitylen; i++)
					{
						if ( mode == PTOA_MACHINE )
						{
							printf("%u:%u",msg.extcommunity.data[i].ip, msg.extcommunity.data[i].num);
							if ( i < msg.announce6.extcommunitylen-1 ) printf(" ");
						}
						else if ( mode == PTOA_JSON )
						{
							printf("\"%u:%u\"",msg.extcommunity.data[i].ip, msg.extcommunity.data[i].num);
							if ( i < msg.announce6.extcommunitylen-1 ) printf(", ");
						}
						else
						{
							struct in_addr addr;
							addr.s_addr = msg.extcommunity.data[i].ip;
							printf(" %s:%u", inet_ntoa(addr), msg.extcommunity.data[i].num);
						}
					}

					if ( mode == PTOA_JSON )
						printf(" ]");
				}

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
