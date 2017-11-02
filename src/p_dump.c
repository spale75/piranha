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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>

#include <p_defs.h>
#include <p_dump.h>
#include <p_tools.h>

/* opening file */
void p_dump_open_file(struct peer_t *peer, int id, struct timeval *ts)
{
	struct tm *tm;
	struct stat sb;
	char dirname[1024];
	char filename[1024];
	char mytime[100];

	peer[id].filets = ts->tv_sec - ( ts->tv_sec % DUMPINTERVAL );

	tm = gmtime((time_t*)&peer[id].filets);
	strftime(mytime, sizeof(mytime), "%Y%m%d%H%M%S" , tm);

	snprintf(dirname, sizeof(dirname), "%s/%s",
		DUMPDIR,
		peer[id].af == 4 ? p_tools_ip4str(id, &peer[id].ip4) : p_tools_ip6str(id, &peer[id].ip6));

	snprintf(peer[id].filename, sizeof(peer[id].filename), "%s/%s/%s",
		DUMPDIR,
		peer[id].af == 4 ? p_tools_ip4str(id, &peer[id].ip4) : p_tools_ip6str(id, &peer[id].ip6),
		mytime);

	snprintf(filename, sizeof(filename), "%s/%s/%s",
		DUMPDIR,
		peer[id].af == 4 ? p_tools_ip4str(id, &peer[id].ip4) : p_tools_ip6str(id, &peer[id].ip6),
		"temp.dump");

	#ifdef DEBUG
	printf("opening '%s'\n",peer[id].filename);
	#endif
	if ( stat(dirname, &sb) == -1 )
	{
		mkdir(dirname, 0755);
	}

	peer[id].fh = fopen(filename, "wb" );
	peer[id].empty = 1;
}

/* log keepalive msg */
void p_dump_add_keepalive(struct peer_t *peer, int id, struct timeval *ts)
{
	p_dump_check_file(peer,id,ts);

	if ( peer[id].fh == NULL ) { return; }
	peer[id].empty = 0;
	{
		struct dump_msg msg;

		msg.type = DUMP_KEEPALIVE;
		msg.ts   = htobe64((uint64_t)ts->tv_sec);
		msg.uts  = htobe64((uint64_t)ts->tv_usec);
		msg.len  = htobe16(0);

		fwrite(&msg, sizeof(msg), 1, peer[id].fh);
	}
}

/* log session close */
void p_dump_add_close(struct peer_t *peer, int id, struct timeval *ts)
{
	if ( peer[id].fh == NULL ) { return; }
	peer[id].empty = 0;
	{
		struct dump_msg msg;

		msg.type = DUMP_CLOSE;
		msg.ts   = htobe64((uint64_t)ts->tv_sec);
		msg.uts  = htobe64((uint64_t)ts->tv_usec);
		msg.len  = htobe16(0);

		fwrite(&msg, sizeof(msg), 1, peer[id].fh);
	}

	p_dump_check_file(peer,id,ts);
}

/* log session open */
void p_dump_add_open(struct peer_t *peer, int id, struct timeval *ts)
{
	p_dump_check_file(peer,id,ts);

	if ( peer[id].fh == NULL ) { return; }
	peer[id].empty = 0;
	{
		struct dump_msg msg;

		msg.type = DUMP_OPEN;
		msg.ts   = htobe64((uint64_t)ts->tv_sec);
		msg.uts  = htobe64((uint64_t)ts->tv_usec);
		msg.len  = htobe16(0);

		fwrite(&msg, sizeof(msg), 1, peer[id].fh);
	}
}

/* footer for each EOF */
void p_dump_add_footer(struct peer_t *peer, int id, struct timeval *ts)
{
	if ( peer[id].fh == NULL ) { return; }
	{
		struct dump_msg msg;

		msg.type = DUMP_FOOTER;
		msg.ts   = htobe64((uint64_t)ts->tv_sec);
		msg.uts  = htobe64((uint64_t)ts->tv_usec);
		msg.len  = htobe64(0);

		fwrite(&msg, sizeof(msg), 1, peer[id].fh);
	}
}

/* log bgp IPv4 withdrawn msg */
void p_dump_add_withdrawn4(struct peer_t *peer, int id, struct timeval *ts, uint32_t prefix, uint8_t mask)
{
	p_dump_check_file(peer,id,ts);

	if ( peer[id].fh == NULL ) { return; }
	peer[id].empty = 0;
	{
		struct dump_msg msg;
		struct dump_withdrawn4 withdrawn;

		msg.type = DUMP_WITHDRAWN4;
		msg.ts   = htobe64((uint64_t)ts->tv_sec);
		msg.uts  = htobe64((uint64_t)ts->tv_usec);

		withdrawn.mask   = mask;
		withdrawn.prefix = htobe32(prefix);

		msg.len = htobe16(sizeof(withdrawn));

		fwrite(&msg, sizeof(msg), 1, peer[id].fh);
		fwrite(&withdrawn, sizeof(withdrawn), 1, peer[id].fh);

	}
}

/* log bgp IPv6 withdrawn msg */
void p_dump_add_withdrawn6(struct peer_t *peer, int id, struct timeval *ts, uint8_t prefix[16], uint8_t mask)
{
	p_dump_check_file(peer,id,ts);

	if ( peer[id].fh == NULL ) { return; }
	peer[id].empty = 0;
	{
		struct dump_msg msg;
		struct dump_withdrawn6 withdrawn;

		msg.type = DUMP_WITHDRAWN6;
		msg.ts   = htobe64((uint64_t)ts->tv_sec);
		msg.uts  = htobe64((uint64_t)ts->tv_usec);

		withdrawn.mask   = mask;

		memcpy(withdrawn.prefix, prefix, sizeof(withdrawn.prefix));

		msg.len = htobe16(sizeof(withdrawn));

		fwrite(&msg, sizeof(msg), 1, peer[id].fh);
		fwrite(&withdrawn, sizeof(withdrawn), 1, peer[id].fh);

	}
}

/* log IPv4 bgp announce msg */
void p_dump_add_announce4(struct peer_t *peer, int id, struct timeval *ts,
			uint32_t prefix,      uint8_t mask,
			uint8_t origin,       uint32_t nexthop,
			void *aspath,         uint16_t aspathlen,
			void *community,      uint16_t communitylen,
			void *extcommunity4,  uint16_t extcommunitylen4,
			void *largecommunity, uint16_t largecommunitylen )
{
	p_dump_check_file(peer,id,ts);

	if ( peer[id].fh == NULL ) { return; }
	peer[id].empty = 0;
	{
		struct dump_msg                     msg;
		struct dump_announce4               announce;
		struct dump_announce_aspath         opt_aspath;
		struct dump_announce_community      opt_community;
		struct dump_announce_extcommunity4  opt_extcommunity4;
		struct dump_announce_largecommunity opt_largecommunity;

		msg.type = DUMP_ANNOUNCE4;
		msg.ts   = htobe64((uint64_t)ts->tv_sec);
		msg.uts  = htobe64((uint64_t)ts->tv_usec);
		msg.len  = htobe16(sizeof(announce)
			+ sizeof(opt_aspath.data[0]) * aspathlen 
			+ sizeof(opt_community.data[0]) * communitylen
			+ sizeof(opt_extcommunity4.data[0]) * extcommunitylen4
			+ sizeof(opt_largecommunity.data[0]) * largecommunitylen );

		announce.mask              = mask;
		announce.prefix            = htobe32(prefix);
		announce.origin            = origin;
		announce.nexthop           = htobe32(nexthop);
		announce.aspathlen         = aspathlen;
		announce.communitylen      = communitylen;
		announce.extcommunitylen4  = extcommunitylen4;
		announce.largecommunitylen = largecommunitylen;

		#ifdef DEBUG
		{
			struct in_addr addr;
			addr.s_addr = htonl(announce.prefix);
			printf("DUMP ANNOUNCE %s/%u\n",p_tools_ip4str(id, &addr),announce.mask);
		}
		#endif

		if ( aspathlen > 0 )
		{
			int i;
			for(i=0; i<aspathlen; i++)
			{
				if ( peer[id].as4 )
					opt_aspath.data[i] = *((uint32_t*)aspath+i);
				else
					opt_aspath.data[i] = *((uint16_t*)aspath+i);
			}
		}

		if ( communitylen > 0 )
		{
			int i;
			for(i=0; i<communitylen; i++)
			{
				opt_community.data[i].asn = *((uint16_t*)community+(i*2));
				opt_community.data[i].num = *((uint16_t*)community+(i*2)+1);
			}
		}

		if ( extcommunitylen4 > 0 )
		{
			int i;
			for(i=0; i<extcommunitylen4; i++)
			{
				opt_extcommunity4.data[i].type    = *((uint8_t*)extcommunity4+(i*8));
				opt_extcommunity4.data[i].subtype = *((uint8_t*)extcommunity4+(i*8)+1);
				memcpy(opt_extcommunity4.data[i].value, (uint8_t*)extcommunity4+(i*8)+2, 6);
			}
		}

		if ( largecommunitylen > 0 )
		{
			int i;
			for(i=0; i<largecommunitylen; i++)
			{
				opt_largecommunity.data[i].global = *((uint32_t*)largecommunity+(i*12));
				opt_largecommunity.data[i].local1 = *((uint32_t*)largecommunity+(i*12)+1);
				opt_largecommunity.data[i].local2 = *((uint32_t*)largecommunity+(i*12)+1);
			}
		}


		fwrite(&msg, sizeof(msg), 1, peer[id].fh);
		fwrite(&announce, sizeof(announce), 1, peer[id].fh);

		if ( aspathlen > 0 )
			fwrite(&opt_aspath, sizeof(opt_aspath.data[0]), aspathlen, peer[id].fh);

		if ( communitylen > 0 )
			fwrite(&opt_community, sizeof(opt_community.data[0]), communitylen, peer[id].fh);

		if ( extcommunitylen4 > 0 )
			fwrite(&opt_extcommunity4, sizeof(opt_extcommunity4.data[0]), extcommunitylen4, peer[id].fh);

		if ( largecommunitylen > 0 )
			fwrite(&opt_largecommunity, sizeof(opt_largecommunity.data[0]), largecommunitylen, peer[id].fh);
	}
}
/* log IPv6 bgp announce msg */
void p_dump_add_announce6(struct peer_t *peer, int id, struct timeval *ts,
			uint8_t prefix[16],   uint8_t mask,
			uint8_t origin,       uint8_t nexthop[16],
			void *aspath,         uint16_t aspathlen,
			void *community,      uint16_t communitylen,
			void *extcommunity6,  uint16_t extcommunitylen6,
			void *largecommunity, uint16_t largecommunitylen )
{
	p_dump_check_file(peer,id,ts);

	if ( peer[id].fh == NULL ) { return; }
	peer[id].empty = 0;
	{
		struct dump_msg                     msg;
		struct dump_announce6               announce;
		struct dump_announce_aspath         opt_aspath;
		struct dump_announce_community      opt_community;
		struct dump_announce_extcommunity6  opt_extcommunity6;
		struct dump_announce_largecommunity opt_largecommunity;

		msg.type = DUMP_ANNOUNCE6;
		msg.ts   = htobe64((uint64_t)ts->tv_sec);
		msg.uts  = htobe64((uint64_t)ts->tv_usec);
		msg.len  = htobe16( sizeof(announce)
			+ sizeof(opt_aspath.data[0]) * aspathlen 
			+ sizeof(opt_community.data[0]) * communitylen
			+ sizeof(opt_extcommunity6.data[0]) * extcommunitylen6
			+ sizeof(opt_largecommunity.data[0]) * largecommunitylen );

		memcpy(announce.prefix, prefix, sizeof(announce.prefix));
		announce.mask              = mask;
		announce.origin            = origin;
		memcpy(announce.nexthop, nexthop, sizeof(announce.nexthop));
		announce.aspathlen         = aspathlen;
		announce.communitylen      = communitylen;
		announce.extcommunitylen6  = extcommunitylen6;
		announce.largecommunitylen = largecommunitylen;

		#ifdef DEBUG
		{
			struct in6_addr addr;
			memcpy(addr.s6_addr, announce.prefix, sizeof(announce.prefix));
			printf("DUMP ANNOUNCE %s/%u\n",p_tools_ip6str(id, &addr),announce.mask);
		}
		#endif

		if ( aspathlen > 0 )
		{
			int i;
			for(i=0; i<aspathlen; i++)
			{
				if ( peer[id].as4 )
					opt_aspath.data[i] = *((uint32_t*)aspath+i);
				else
					opt_aspath.data[i] = *((uint16_t*)aspath+i);
			}
		}

		if ( communitylen > 0 )
		{
			int i;
			for(i=0; i<communitylen; i++)
			{
				opt_community.data[i].asn = *((uint16_t*)community+(i*2));
				opt_community.data[i].num = *((uint16_t*)community+(i*2)+1);
			}
		}

		if ( extcommunitylen6 > 0 )
		{
			int i;
			for(i=0; i<extcommunitylen6; i++)
			{
				opt_extcommunity6.data[i].type    = *((uint8_t*)extcommunity6+(i*20));
				opt_extcommunity6.data[i].subtype = *((uint8_t*)extcommunity6+(i*20)+1);
				memcpy(opt_extcommunity6.data[i].global, (uint8_t*)extcommunity6+(i*8)+2, 16);
				opt_extcommunity6.data[i].local = *((uint16_t*)(uint8_t*)extcommunity6+(i*8)+18);
			}
		}

		if ( largecommunitylen > 0 )
		{
			int i;
			for(i=0; i<largecommunitylen; i++)
			{
				opt_largecommunity.data[i].global = *((uint32_t*)largecommunity+(i*12));
				opt_largecommunity.data[i].local1  = *((uint32_t*)largecommunity+(i*12)+1);
				opt_largecommunity.data[i].local2  = *((uint32_t*)largecommunity+(i*12)+1);
			}
		}

		fwrite(&msg, sizeof(msg), 1, peer[id].fh);
		fwrite(&announce, sizeof(announce), 1, peer[id].fh);

		if ( aspathlen > 0 )
			fwrite(&opt_aspath, sizeof(opt_aspath.data[0]), aspathlen, peer[id].fh);

		if ( communitylen > 0 )
			fwrite(&opt_community, sizeof(opt_community.data[0]), communitylen, peer[id].fh);

		if ( extcommunitylen6 > 0 )
			fwrite(&opt_extcommunity6, sizeof(opt_extcommunity6.data[0]), extcommunitylen6, peer[id].fh);

		if ( largecommunitylen > 0 )
			fwrite(&opt_largecommunity, sizeof(opt_largecommunity.data[0]), largecommunitylen, peer[id].fh);

	}
}

/* check if need to reopen a new file */
void p_dump_check_file(struct peer_t *peer, int id, struct timeval *ts)
{
	uint64_t mts = ts->tv_sec - ( ts->tv_sec % DUMPINTERVAL );

	if ( mts == peer[id].filets && peer[id].fh != NULL ) { return; }

	if ( mts != peer[id].filets )
	{
		if ( peer[id].fh != NULL )
		{
			p_dump_add_footer(peer,id,ts);
			p_dump_close_file(peer,id);
		}

		if ( peer[id].status != 0 )
		{
			p_dump_open_file(peer,id,ts);

			if ( peer[id].af == 4 )
				p_dump_add_header4(peer,id,ts);
			else
				p_dump_add_header6(peer,id,ts);
		}
	}
	else if ( peer[id].fh == NULL && peer[id].status != 0)
	{
		p_dump_open_file(peer,id,ts);
		if ( peer[id].af == 4 )
			p_dump_add_header4(peer,id,ts);
		else
			p_dump_add_header6(peer,id,ts);
	}
}

/* file header */
void p_dump_add_header4(struct peer_t *peer, int id, struct timeval *ts)
{
	if ( peer[id].fh == NULL ) { return; }
	{
		struct dump_msg msg;
		struct dump_header4 header;

		msg.type = DUMP_HEADER4;
		msg.ts   = htobe64((uint64_t)ts->tv_sec);
		msg.uts  = htobe64((uint64_t)ts->tv_usec);
		msg.len  = htobe16(sizeof(header));

		header.ip   = peer[id].ip4.s_addr;
		header.as   = htobe32(peer[id].as);
		header.type = peer[id].type;

		fwrite(&msg,    sizeof(msg),    1, peer[id].fh);
		fwrite(&header, sizeof(header), 1, peer[id].fh);
	}
}

/* file header */
void p_dump_add_header6(struct peer_t *peer, int id, struct timeval *ts)
{
	if ( peer[id].fh == NULL ) { return; }
	{
		struct dump_msg msg;
		struct dump_header6 header;

		msg.type = DUMP_HEADER6;
		msg.ts   = htobe64((uint64_t)ts->tv_sec);
		msg.uts  = htobe64((uint64_t)ts->tv_usec);
		msg.len  = htobe16(sizeof(header));

		memcpy(header.ip, peer[id].ip6.s6_addr, sizeof(header.ip));
		header.as   = htobe32(peer[id].as);
		header.type = peer[id].type;

		fwrite(&msg,    sizeof(msg),    1, peer[id].fh);
		fwrite(&header, sizeof(header), 1, peer[id].fh);
	}
}

/* close file */
void p_dump_close_file(struct peer_t *peer, int id)
{
	char filename[1024];

	if ( peer[id].fh == NULL ) { return; }

	fclose(peer[id].fh);

	snprintf(filename, sizeof(filename), "%s/%s/%s",
		DUMPDIR,
		peer[id].af == 4 ? p_tools_ip4str(id, &peer[id].ip4) : p_tools_ip6str(id, &peer[id].ip6),
		"temp.dump");

	rename(filename, peer[id].filename);

	if ( peer[id].empty == 1 )
	{
		unlink(peer[id].filename);
	}
}
