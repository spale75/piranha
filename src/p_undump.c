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
#include <assert.h>


#include <p_defs.h>
#include <p_undump.h>
#include <p_tools.h>

struct dump_file_ctx *p_undump_open(char *file)
{
	struct dump_file_ctx *ctx;

	ctx = malloc(sizeof(struct dump_file_ctx));
	assert(ctx);

	memset(ctx, 0, sizeof(struct dump_file_ctx));

	strcpy(ctx->file, file);

	if ( ( ctx->fh = fopen(file, "r") ) == NULL )
	{
		free(ctx);
		return NULL;
	}

	return ctx;
}

int p_undump_close(struct dump_file_ctx *ctx)
{
	assert(ctx);

	if ( ctx->fh )
		fclose(ctx->fh);

	free(ctx);

	return (0);
}

int p_undump_readmsg(struct dump_file_ctx *ctx, struct dump_full_msg *fmsg)
{
	char buffer[65536];
	struct dump_msg msg;
	int len;

	if ( ctx == NULL )
		return (-1);

	if ( ctx->fh == NULL )
		return (-1);

	if ( fmsg == NULL )
		return (-1);

	if ( ctx->end )
		return (-1);

	if ( ( len = fread(&msg, 1, sizeof(msg), ctx->fh) ) != sizeof(msg) )
		return (-1);

	#ifdef DEBUG
	// p_tools_dump("Header Dump", (char*)&msg, sizeof(msg));
	#endif

	/* convert header fields */
	msg.len = be16toh(msg.len);
	msg.ts  = be64toh(msg.ts);
	msg.uts = be64toh(msg.uts);
	memcpy(&fmsg->msg, &msg, sizeof(msg));

	#ifdef DEBUG
	printf("type %u len %u timestamp %llu\n",msg.type,msg.len,(unsigned long long int)msg.ts);
	#endif


	memset(buffer, 0, sizeof(buffer));

	if ( msg.len && ( len = fread(buffer, 1, msg.len, ctx->fh) ) != msg.len )
		return (-1);

	#ifdef DEBUG
	// p_tools_dump("Message Dump", buffer, msg.len);
	#endif

	if ( msg.type == DUMP_HEADER4 && !ctx->head )
	{
		struct dump_header4 *header = (struct dump_header4 *)(buffer);
		fmsg->header4.ip   = be32toh(header->ip);
		fmsg->header4.as   = be32toh(header->as);
		fmsg->header4.type = header->type;
		ctx->head=1;
	}
	else if ( msg.type == DUMP_HEADER6 && !ctx->head )
	{
		struct dump_header6 *header = (struct dump_header6 *)(buffer);
		memcpy(fmsg->header6.ip,header->ip, sizeof(header->ip));
		fmsg->header6.as   = be32toh(header->as);
		fmsg->header6.type = header->type;
		ctx->head=1;
	}
	else if ( msg.type == DUMP_OPEN && ctx->head )
	{
	}
	else if ( msg.type == DUMP_CLOSE && ctx->head )
	{
	}
	else if ( msg.type == DUMP_KEEPALIVE && ctx->head )
	{
	}
	else if ( ( msg.type == DUMP_ANNOUNCE4 || msg.type == DUMP_ANNOUNCE6 ) && ctx->head )
	{
		int jump = 0;
		struct dump_announce4               *announce4 = NULL;
		struct dump_announce6               *announce6 = NULL;
		struct dump_announce_aspath         *aspath;
		struct dump_announce_community      *community;
		struct dump_announce_extcommunity4  *extcommunity4;
		struct dump_announce_extcommunity6  *extcommunity6;
		struct dump_announce_largecommunity *largecommunity;
		uint16_t aspathlen         = 0;
		uint16_t communitylen      = 0;
		uint16_t extcommunitylen4  = 0;
		uint16_t extcommunitylen6  = 0;
		uint16_t largecommunitylen = 0;

		if ( msg.type == DUMP_ANNOUNCE4 )
		{
			announce4 = (struct dump_announce4*)buffer;
			jump += sizeof(struct dump_announce4);
	
			fmsg->announce4.mask              = announce4->mask;
			fmsg->announce4.prefix            = be32toh(announce4->prefix);
			fmsg->announce4.origin            = announce4->origin;
			fmsg->announce4.nexthop           = be32toh(announce4->nexthop);
			fmsg->announce4.aspathlen         = announce4->aspathlen;
			fmsg->announce4.communitylen      = announce4->communitylen;
			fmsg->announce4.extcommunitylen4  = announce4->extcommunitylen4;
			fmsg->announce4.largecommunitylen = announce4->largecommunitylen;

			aspathlen         = fmsg->announce4.aspathlen;
			communitylen      = fmsg->announce4.communitylen;
			extcommunitylen4  = fmsg->announce4.extcommunitylen4;
			largecommunitylen = fmsg->announce4.largecommunitylen;
	
		}
		else if ( msg.type == DUMP_ANNOUNCE6 )
		{
			announce6 = (struct dump_announce6*)buffer;
			jump += sizeof(struct dump_announce6);
	
			memcpy(fmsg->announce6.prefix, announce6->prefix, sizeof(announce6->prefix));
			fmsg->announce6.mask              = announce6->mask;
			fmsg->announce6.origin            = announce6->origin;
			memcpy(fmsg->announce6.nexthop, announce6->nexthop, sizeof(announce6->nexthop));
			fmsg->announce6.aspathlen         = announce6->aspathlen;
			fmsg->announce6.communitylen      = announce6->communitylen;
			fmsg->announce6.extcommunitylen6  = announce6->extcommunitylen6;
			fmsg->announce6.largecommunitylen = announce6->largecommunitylen;
	
			aspathlen         = fmsg->announce6.aspathlen;
			communitylen      = fmsg->announce6.communitylen;
			extcommunitylen6  = fmsg->announce6.extcommunitylen6;
			largecommunitylen = fmsg->announce6.largecommunitylen;
		}
	
		if ( aspathlen > 0 )
		{
			int i;
			aspath = (struct dump_announce_aspath*) (buffer+jump);
			jump += sizeof(aspath->data[0]) * aspathlen;
	
			for(i=0; i<aspathlen; i++)
				fmsg->aspath.data[i] = be32toh(aspath->data[i]);
		}

		if ( communitylen > 0 )
		{
			int i;
			community = (struct dump_announce_community*) (buffer+jump);
			jump += sizeof(community->data[0]) * communitylen;

			for(i=0; i<communitylen; i++)
			{
				fmsg->community.data[i].asn = be16toh(community->data[i].asn);
				fmsg->community.data[i].num = be16toh(community->data[i].num);
			}
		}

		if ( extcommunitylen4 > 0 )
		{
			int i;
			extcommunity4 = (struct dump_announce_extcommunity4*) (buffer+jump);
			jump += sizeof(extcommunity4->data[0]) * extcommunitylen4;

			for(i=0; i<extcommunitylen4; i++)
			{
				fmsg->extcommunity4.data[i].type     = extcommunity4->data[i].type;
				fmsg->extcommunity4.data[i].subtype  = extcommunity4->data[i].subtype;
				memcpy(fmsg->extcommunity4.data[i].value, extcommunity4->data[i].value, 6);
			}
		}

		if ( extcommunitylen6 > 0 )
		{
			int i;
			extcommunity6 = (struct dump_announce_extcommunity6*) (buffer+jump);
			jump += sizeof(extcommunity6->data[0]) * extcommunitylen6;

			for(i=0; i<extcommunitylen6; i++)
			{
				fmsg->extcommunity6.data[i].type     = extcommunity6->data[i].type;
				fmsg->extcommunity6.data[i].subtype  = extcommunity6->data[i].subtype;
				memcpy(fmsg->extcommunity6.data[i].global, extcommunity6->data[i].global, 16);
				fmsg->extcommunity6.data[i].local    = be16toh(extcommunity6->data[i].local);
			}
		}

		if ( largecommunitylen > 0 )
		{
			int i;
			largecommunity = (struct dump_announce_largecommunity*) (buffer+jump);
			jump += sizeof(largecommunity->data[0]) * largecommunitylen;

			for(i=0; i<largecommunitylen; i++)
			{
				fmsg->largecommunity.data[i].global = be32toh(largecommunity->data[i].global);
				fmsg->largecommunity.data[i].local1 = be32toh(largecommunity->data[i].local1);
				fmsg->largecommunity.data[i].local2 = be32toh(largecommunity->data[i].local2);
			}
		}
	}
	else if ( msg.type == DUMP_WITHDRAWN4 && ctx->head )
	{
		struct dump_withdrawn4 *withdrawn4 = (struct dump_withdrawn4*)buffer;

		fmsg->withdrawn4.mask = withdrawn4->mask;
		fmsg->withdrawn4.prefix = be32toh(withdrawn4->prefix);
	}
	else if ( msg.type == DUMP_WITHDRAWN6 && ctx->head )
	{
		struct dump_withdrawn6 *withdrawn6 = (struct dump_withdrawn6*)buffer;

		fmsg->withdrawn6.mask = withdrawn6->mask;
		memcpy(fmsg->withdrawn6.prefix, withdrawn6->prefix, sizeof(withdrawn6->prefix));
	}
	else if ( msg.type == DUMP_FOOTER && ctx->head )
	{
		ctx->end=1;
	}
	else
	{
		return (-1);
	}

	return 0;
}
