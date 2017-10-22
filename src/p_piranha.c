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
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <netdb.h>
#include <dirent.h>
#include <limits.h>


#include <p_defs.h>
#include <p_piranha.h>
#include <p_log.h>
#include <p_config.h>
#include <p_socket.h>
#include <p_dump.h>
#include <p_tools.h>


/* init the global structures */
struct config_t config;
struct peer_t   peer[MAX_PEERS];
struct timeval  ts;


/* 00 BEGIN ;) */
int main(int argc, char *argv[])
{
	#ifdef DEBUG
	/* say hello */
	printf("Piranha v%s.%s.%s BGP Daemon, Copyright(c) 2004-2017 Pascal Gloor\n",P_VER_MA,P_VER_MI,P_VER_PL);
	#endif

	#ifdef DEBUG
	setlinebuf(stdout);
	#endif


	/* set initial time */
	gettimeofday(&ts,NULL);

	{
		char logline[100];
		snprintf(logline, sizeof(logline), "Piranha v%s.%s.%s started.\n",P_VER_MA,P_VER_MI,P_VER_PL);
		p_log_add((time_t)ts.tv_sec, logline);
	}
	
	/* check the cmd line options */
	if ( argc != 2 ) { p_main_syntax(argv[0]); return -1; }

	/* init some stuff and load the config */
	config.file = argv[1];

	if ( p_config_load((struct config_t*)&config,(struct peer_t*)peer, (time_t)ts.tv_sec) == -1 )
	{ fprintf(stderr,"error while parsing configuration file %s\n", config.file); return -1; }

	/* chown working dir */
	mychown(PATH, config.uid, config.gid, 0);

	/* set config reload for signal HUP */
	signal(SIGHUP, p_main_sighup);

	/* init the socket */
	if ( p_socket_start((struct config_t*)&config, (struct peer_t*)&peer) == -1 )
	{
		fprintf(stderr,"socket error, aborting\n");
	 	return -1;
	}


	#ifndef DEBUG
	/* we dont use daemon() here, it doesnt exist on solaris/suncc ;-) */
	/* daemon(1,0); */
	if ( mydaemon(1,0) ) { fprintf(stderr,"daemonization error.\n"); }
	#endif

	/* log the pid */
	p_log_pid();

	while ( p_main_loop() == 0 )
	{
		#ifdef DEBUG
		printf("accept() loop\n");
		#endif

		/* we want to sessions to come up slowly */
		/* therefor we sleep a bit here. */
		usleep(100000);

		p_log_status((struct config_t*)&config,(struct peer_t*)peer, (time_t)ts.tv_sec);

		/* we update a global var with the actual timestamp */
		gettimeofday(&ts,NULL);
	}

	#ifdef DEBUG
	printf("accept() failed, aborting\n");
	#endif

	return -1;
}

/*  check for accept() and start thread() */
int p_main_loop()
{
	int sock;

	if ( ( sock = p_socket_accept((struct config_t*)&config) ) == -1 )
	{
		return 0;
	}
	else
	{
		pthread_t thread;
		int *mysock;
		mysock = malloc(sizeof(int));
		memcpy(mysock,&sock,sizeof(int));

		pthread_create(&thread, NULL, p_main_peer, (void *)mysock);
	}
	return 0;
}

/* peer thread */
void *p_main_peer(void *data)
{
	int sock;
	int a;
	int allow = 0;
	int peerid = -1;
	struct sockaddr_storage sockaddr;
	struct sockaddr_in  *addr4 = (struct sockaddr_in  *)&sockaddr;
	struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&sockaddr;
	socklen_t socklen = sizeof(sockaddr);

	memcpy(&sock,data,sizeof(sock));

	#ifdef DEBUG
	printf("I'm thready, my socket is %i\n",sock);
	#endif

	if ( getpeername(sock,(struct sockaddr*)&sockaddr, &socklen) == -1 )
	{
		#ifdef DEBUG
		printf("failed to get peeraddr!\n");
		#endif
		p_main_peer_exit(data,sock);
	}

	#ifdef DEBUG
	if ( sockaddr.ss_family == AF_INET )
	{
		char str[INET_ADDRSTRLEN];
		printf("peer ip is: %s\n", inet_ntop(AF_INET, &addr4->sin_addr, str, sizeof(str)));
	}
	else if ( sockaddr.ss_family == AF_INET6 )
	{
		char str[INET6_ADDRSTRLEN];
		printf("peer ip is: %s\n", inet_ntop(AF_INET6, &addr6->sin6_addr, str, sizeof(str)));
	}
	else
	{
		printf("peer has an unsupported address family! (%i)\n", sockaddr.ss_family);
	}
	#endif

	/* does the peer exist? */
	for(a=0; a<MAX_PEERS; a++)
	{
		if (
			( sockaddr.ss_family == AF_INET  && peer[a].af == 4 && p_tools_sameip4(&peer[a].ip4, &addr4->sin_addr)  && peer[a].allow == 1 ) ||
			( sockaddr.ss_family == AF_INET6 && peer[a].af == 6 && p_tools_sameip6(&peer[a].ip6, &addr6->sin6_addr) && peer[a].allow == 1 )
		) {
			char logline[100];
			if ( peer[a].status == 0 )
			{
				snprintf(logline,sizeof(logline), "%s connection (known)\n",
					sockaddr.ss_family == AF_INET ? p_tools_ip4str(a, &peer[a].ip4) : p_tools_ip6str(a, &peer[a].ip6) );
				p_log_add((time_t)ts.tv_sec, logline);
				#ifdef DEBUG
				printf("peer ip allowed id %i\n",a);
				#endif
				allow          = 1;
				peer[a].sock   = sock;
				peer[a].status = 1;
				peer[a].rhold  = BGP_DEFAULT_HOLD;
				peer[a].shold  = BGP_DEFAULT_HOLD;
				peer[a].ilen   = 0;
				peer[a].olen   = 0;
				peer[a].rts    = ts.tv_sec;
				peer[a].sts    = ts.tv_sec;
				peer[a].cts    = ts.tv_sec;
				peer[a].rmsg   = 0;
				peer[a].smsg   = 0;
				peer[a].filets = 0;
				peer[a].fh     = NULL;
				peer[a].ucount = 0;
				peer[a].as4    = 0;
				peerid         = a;

			}
			else
			{
				snprintf(logline, sizeof(logline), "%s connection (already connected)\n",
					sockaddr.ss_family == AF_INET ? p_tools_ip4str(a, &peer[a].ip4) : p_tools_ip6str(a, &peer[a].ip6) );
				p_log_add((time_t)ts.tv_sec, logline);
			}

			break;
		}
	}

	if ( allow == 0 )
	{
		char logline[100 + INET6_ADDRSTRLEN];
		if ( sockaddr.ss_family == AF_INET )
			inet_ntop(AF_INET, &addr4->sin_addr, logline, INET6_ADDRSTRLEN );
		else
			inet_ntop(AF_INET6, &addr6->sin6_addr, logline, INET6_ADDRSTRLEN );
	
		snprintf(logline + strlen(logline), sizeof(logline) - strlen(logline), " connection (unknown)\n");
		p_log_add((time_t)ts.tv_sec, logline);

		p_main_peer_exit(data,sock);
	}

	#ifdef DEBUG
	printf("peerid %i\n",peerid);
	#endif

	p_main_peer_loop(peerid);
	p_dump_add_close(peer, peerid, ts.tv_sec);

	/* peer disconnected, lets wait a bit to avoid direct reconnection */
	/* we'll wait DUMPINTERVAL time! */

	peer[peerid].status = 1;
	sleep(DUMPINTERVAL);
	peer[peerid].status = 0;

	p_main_peer_exit(data, sock);

	return NULL;
}

/* peer looop */
void p_main_peer_loop(int id)
{
	char logline[100];
	char *ibuf;
	char *obuf;
	uint8_t marker[16];

	{ int a; for(a=0; a<sizeof(marker); a++) { marker[a] = 0xff; } }

	peer[id].ilen = 0;
	peer[id].olen = 0;

	ibuf = malloc(INPUT_BUFFER);
	obuf = malloc(OUTPUT_BUFFER);

	#ifdef DEBUG
	printf("peer status %u\n",peer[id].status);
	printf("starting peer loop\n");
	#endif

	p_main_peer_open(id, obuf);

	while((peer[id].status>0))
	{
		/* receiving new datas, sleep 1sec if nothing */
		int tlen = 0;
		int maxlen = INPUT_BUFFER - peer[id].ilen;

		if ( TEMP_BUFFER < maxlen ) { maxlen = TEMP_BUFFER; }

		tlen = recv(peer[id].sock, ibuf+peer[id].ilen, INPUT_BUFFER-peer[id].ilen, 0);

		if ( tlen == -1 )
		{
			p_dump_check_file(peer,id,ts.tv_sec);
			sleep(1);
		}
		else if ( tlen > 0 )
		{
			#ifdef DEBUG
			printf("got data\n");
			#endif
			peer[id].ilen += tlen;
		}
		else
		{
			snprintf(logline, sizeof(logline), "%s socket went down\n",
				peer[id].af == 4 ? p_tools_ip4str(id, &peer[id].ip4) : p_tools_ip6str(id, &peer[id].ip6) );
			p_log_add((time_t)ts.tv_sec, logline);
			#ifdef DEBUG
			printf("something failed on recv()\n");
			#endif
			peer[id].status = 0;
		}


		/* working on datas */
		if ( peer[id].ilen > 0 )
		{
			p_main_peer_work(ibuf, obuf, id);
		}

		/* check if the peer timed out  (note: 0 == no keepalive!) */

		if ( peer[id].rhold != 0 && ( (ts.tv_sec - peer[id].rts) > peer[id].rhold ) )
		{
			/* timeout ;( */
			snprintf(logline, sizeof(logline), "%s holdtime expired\n",
				peer[id].af == 4 ? p_tools_ip4str(id, &peer[id].ip4) : p_tools_ip6str(id, &peer[id].ip6) );
			p_log_add((time_t)ts.tv_sec, logline);
			peer[id].status = 0;
		}

		/* time to send a keepalive message ? (note: 0 == no keepalive!) */

		if ( peer[id].shold != 0 && ( (ts.tv_sec - peer[id].sts) > (peer[id].shold / 3) ) )
		{
			/* yeah */
			struct bgp_header r_header;
			memcpy(&r_header, marker, sizeof(marker));
			r_header.len  = htons(BGP_HEADER_LEN);
			r_header.type = 4;

			memcpy(obuf+peer[id].olen, &r_header, BGP_HEADER_LEN);
			peer[id].olen += BGP_HEADER_LEN;

			peer[id].sts = ts.tv_sec;
			peer[id].smsg++;

			p_main_peer_send(id, obuf);
		}


		/* sending */
		while((peer[id].olen>0 && peer[id].status))
		{
			peer[id].smsg++;
			p_main_peer_send(id, obuf);
			if ( peer[id].olen == -1 )
			{
				#ifdef DEBUG
				printf("failed to send!\n");
				#endif
				snprintf(logline, sizeof(logline), "%s failed to send\n",
					peer[id].af == 4 ? p_tools_ip4str(id, &peer[id].ip4) : p_tools_ip6str(id, &peer[id].ip6) );
				p_log_add((time_t)ts.tv_sec, logline);
				peer[id].status = 0;
			}
		}

		if ( peer[id].olen == -1 )
		{
			printf("%s error while sending\n",
				peer[id].af == 4 ? p_tools_ip4str(id, &peer[id].ip4) : p_tools_ip6str(id, &peer[id].ip6) );
		}


	}

	free(ibuf);
	free(obuf);

	snprintf(logline, sizeof(logline), "%s down\n",
		peer[id].af == 4 ? p_tools_ip4str(id, &peer[id].ip4) : p_tools_ip6str(id, &peer[id].ip6) );
	p_log_add((time_t)ts.tv_sec, logline);

	peer[id].cts = ts.tv_sec;

	#ifdef DEBUG
	printf("peer is gone\n");
	#endif
}

/* sending BGP open and first keepalive */
void p_main_peer_open(int id, char *obuf)
{
	/* reply with my open msg */
	struct bgp_header r_header;
	struct bgp_open   r_open;
	struct bgp_param  r_param;
	struct bgp_param_capa *r_capa_as4;
	struct bgp_param_capa *r_capa_afi;
	uint32_t ugly_afi4 = htonl(0x00010001);
	uint32_t ugly_afi6 = htonl(0x00020001);


	uint8_t marker[16];

	{ int a; for(a=0; a<sizeof(marker); a++) { marker[a] = 0xff; } }

	memcpy(r_header.marker, marker, sizeof(r_header.marker));


	/* 4 bytes AS capability RFC6793 */
	r_capa_as4 = (struct bgp_param_capa*)r_param.param;
	r_capa_as4->type     = 65;
	r_capa_as4->len      = 4;
	r_capa_as4->u.as4    = htonl(config.as);

	if ( peer[id].af == 4 )
	{
		/* AFI support IPv4 unicast */
		r_capa_afi = (struct bgp_param_capa*)(r_param.param+r_capa_as4->len+2);
		r_capa_afi->type     = 1;
		r_capa_afi->len      = 4;
		memcpy(r_capa_afi->u.def,&ugly_afi4,4); /* TO BE IMPROVED, UGLY !!!!! */
	}
	else
	{
		/* AFI support IPv6 unicast */
		r_capa_afi = (struct bgp_param_capa*)(r_param.param+r_capa_as4->len+2);
		r_capa_afi->type     = 1;
		r_capa_afi->len      = 4;
		memcpy(r_capa_afi->u.def,&ugly_afi6,4); /* TO BE IMPROVED, UGLY !!!!! */
	}

	/* capabilities parameter */
	r_param.type     = 2;
	r_param.len      = r_capa_as4->len + 2 + r_capa_afi->len + 2;

	/* open message */
	r_open.version   = 4;
	r_open.as        = ( config.as > 65535 ) ? 23456 : htons((uint16_t)config.as);
	r_open.holdtime  = htons(peer[id].shold);
	r_open.bgp_id    = htonl(config.routerid);
	r_open.param_len = r_param.len + 2;

	r_header.len     = htons(BGP_HEADER_LEN+BGP_OPEN_LEN+r_param.len+2);
	r_header.type    = 1;

	memcpy(obuf+peer[id].olen,&r_header, BGP_HEADER_LEN);
	peer[id].olen += BGP_HEADER_LEN;

	memcpy(obuf+peer[id].olen,&r_open, BGP_OPEN_LEN);
	peer[id].olen += BGP_OPEN_LEN;

	memcpy(obuf+peer[id].olen,&r_param, r_param.len+2);
	peer[id].olen += r_param.len+2;


	peer[id].smsg++;
	p_main_peer_send(id,obuf);

	/* we add directly the first keepalive msg */

	r_header.len  = htons(BGP_HEADER_LEN);
	r_header.type = 4;

	memcpy(obuf+peer[id].olen, &r_header, BGP_HEADER_LEN);
	peer[id].olen += BGP_HEADER_LEN;

	/* update the keepalive sent timestamp */
	peer[id].sts = ts.tv_sec;

	/* send the packet */

	peer[id].smsg++;
	p_main_peer_send(id,obuf);
}

/* bgp decoding stuff */
void p_main_peer_work(char *ibuf, char *obuf, int id)
{
	char logline[100];
	uint8_t marker[16] = {	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

	while(1)
	{
		int pos = 0;
		struct bgp_header *header;

		if ( peer[id].ilen < sizeof(struct bgp_header) )
		{
			#ifdef DEBUG
			printf("not all header\n");
			#endif
			return;
		}

		header = (struct bgp_header*)ibuf;

		if ( memcmp(header->marker, marker, sizeof(marker)) != 0 )
		{
			snprintf(logline, sizeof(logline), "%s packet decoding error\n",
				peer[id].af == 4 ? p_tools_ip4str(id, &peer[id].ip4) : p_tools_ip6str(id, &peer[id].ip6) );
			p_log_add((time_t)ts.tv_sec, logline);
			#ifdef DEBUG
			printf("invalid marker\n");
			#endif
			peer[id].status = 0;
			return;
		}

		#ifdef DEBUG
		printf("len: %u type: %u (buffer %u)\n",htons(header->len),header->type,peer[id].ilen);
		#endif

		if ( peer[id].ilen < htons(header->len) )
		{
			#ifdef DEBUG
			printf("bgp message not complete msg len %u, buffer len %u\n",htons(header->len),peer[id].ilen);
			#endif
			return;
		}

		/* show bgp message */
		#ifdef DEBUG
		p_tools_dump("BGP Message", ibuf, htons(header->len));
		#endif


		pos += BGP_HEADER_LEN;
		peer[id].rmsg++;

		peer[id].rts = ts.tv_sec;

		/* BGP OPEN MSG */
		if ( header->type == BGP_OPEN && peer[id].status == 1 )
		{
			struct bgp_open *bopen;
			bopen = (struct bgp_open*) (ibuf + pos);
			int alt_asn = 0;

			if ( bopen->version != 4 )
			{
				snprintf(logline, sizeof(logline), "%s wrong bgp version (%u)\n",
					peer[id].af == 4 ? p_tools_ip4str(id, &peer[id].ip4) : p_tools_ip6str(id, &peer[id].ip6),
					bopen->version);
				p_log_add((time_t)ts.tv_sec, logline);
				#ifdef DEBUG
				printf("invalid BGP version %u\n",bopen->version);
				#endif
				peer[id].status = 0;
				return;
			}
			#ifdef DEBUG
			printf("BGP Version: %u\n",bopen->version);
			#endif

			if ( htons(bopen->as) == 23456 ) /* AS_TRANS RFC6793 */
			{
				alt_asn=1;
			}
			else if ( htons(bopen->as) != peer[id].as )
			{
				snprintf(logline, sizeof(logline), "%s wrong neighbor as (%u != %u)\n",
					peer[id].af == 4 ? p_tools_ip4str(id, &peer[id].ip4) : p_tools_ip6str(id, &peer[id].ip6),
					htons(bopen->as),peer[id].as);
				p_log_add((time_t)ts.tv_sec, logline);
				#ifdef DEBUG
				printf("invalid BGP neighbor AS %u\n",htons(bopen->as));
				#endif
				peer[id].status = 0;
				return;
			}

			peer[id].rhold = htons(bopen->holdtime);

			if ( peer[id].rhold < peer[id].shold )
			{
				peer[id].shold = peer[id].rhold;
			}

			if ( htons(header->len) != BGP_HEADER_LEN + BGP_OPEN_LEN + bopen->param_len )
			{
				snprintf(logline, sizeof(logline), "%s parameter parsing error)\n",
					peer[id].af == 4 ? p_tools_ip4str(id, &peer[id].ip4) : p_tools_ip6str(id, &peer[id].ip6) );
				p_log_add((time_t)ts.tv_sec, logline);
				#ifdef DEBUG
				printf("size error in bgp open params, len %u, header %u open %u param %u\n",htons(header->len),BGP_HEADER_LEN,BGP_OPEN_LEN,bopen->param_len);
				#endif
				peer[id].status = 0;
				return;
			}

			pos += BGP_OPEN_LEN;

			while(pos<htons(header->len))
			{
				struct bgp_param *param;
				param = (struct bgp_param*) (ibuf+pos);
				#ifdef DEBUG
				printf("param code %u len %u\n",param->type,param->len);
				#endif
				if ( param->type == 2 ) // BGP Capabilities
				{
					int i = 0;
					struct bgp_param_capa *capa;

					while(i<param->len)
					{
						capa = (struct bgp_param_capa*) (param->param + i);

						#ifdef DEBUG
						int b;
						printf("capability %u (%s) len %u : ", capa->type, bgp_capability[capa->type], capa->len);
						for(b=0; b<capa->len; b++)
							printf("%02x ", (uint8_t)capa->u.def[b]);
						printf("\n");
						#endif

						if ( capa->type == 65 && capa->len == 4 ) /* Support for 4-octets ASN */
						{
							if ( peer[id].as == ntohl(capa->u.as4) )
							{
								alt_asn = 0;
								peer[id].as4=1;
							}
							else
							{
								peer[id].status=0;
								snprintf(logline, sizeof(logline), "%s ASN mismatch in 4-octet capability\n",
									peer[id].af == 4 ? p_tools_ip4str(id, &peer[id].ip4) : p_tools_ip6str(id, &peer[id].ip6) );
								p_log_add((time_t)ts.tv_sec, logline);
								return;
							}
						}
						i += 1 + 1 + capa->len;
					}
				}
				#ifdef DEBUG
				else
				{
					int b;
					for(b=0; b<param->len; b++)
						printf("%u ",(uint8_t)param->param[b]);
					printf("\n");
				}
				#endif
				pos += 2;
				pos += param->len;
				if ( param->type > 4 )
				{
					#ifdef DEBUG
					printf("parameter rejected!\n");
					#endif
					peer[id].status = 0;
					return;
				}
			}

			if ( alt_asn == 1 ) /* AS_TRANS was used but we didn't find capa 65 4 octets ASN */
			{
				peer[id].status=0;
				snprintf(logline, sizeof(logline), "%s AS_TRANS in header but no capa 65 found\n",
					peer[id].af == 4 ? p_tools_ip4str(id, &peer[id].ip4) : p_tools_ip6str(id, &peer[id].ip6) );
				p_log_add((time_t)ts.tv_sec, logline);
				return;
			}

			snprintf(logline, sizeof(logline), "%s established\n",
				peer[id].af == 4 ? p_tools_ip4str(id, &peer[id].ip4) : p_tools_ip6str(id, &peer[id].ip6) );
			p_log_add((time_t)ts.tv_sec, logline);

			peer[id].status = 2;

			p_dump_add_open(peer, id, ts.tv_sec);

		}
		else if ( header->type == BGP_UPDATE && peer[id].status == 2 )
		{
			/* BGP update */
			uint16_t  wlen;
			uint16_t  alen;
			uint8_t   origin = 0xff;
			void     *aspath = NULL;
			uint16_t  aspathlen = 0;
			void     *community = NULL;
			uint16_t  communitylen = 0;
			void     *extcommunity = NULL;
			uint8_t   extcommunitylen = 0;

			wlen = *(uint16_t *) (ibuf + pos);
			wlen = ntohs(wlen);
			pos += 2;

			#ifdef DEBUG
			printf("Withdrawn Length %u\n",wlen);
			#endif

			while(wlen>0)
			{
				uint8_t  plen   = *(uint8_t *)  (ibuf+pos);
				uint32_t prefix = 0;
				int blen = 0;

				pos++;
				wlen--;

				if ( plen > 24 )
				{
					uint32_t o = *(uint8_t *) (ibuf+pos+3);
					prefix += o << 0;
					blen++;
				}
				if ( plen > 16 )
				{
					uint32_t o = *(uint8_t *) (ibuf+pos+2);
					prefix += o << 8;
					blen++;
				}
				if ( plen > 8 )
				{
					uint32_t o = *(uint8_t *) (ibuf+pos+1);
					prefix += o << 16;
					blen++;
				}
				if ( plen > 0 )
				{
					uint32_t o = *(uint8_t *) (ibuf+pos);
					prefix += o << 24;
					blen++;
				}
				pos+=blen;
				wlen-= blen;

				#ifdef DEBUG
				{
					struct in_addr addr;
					addr.s_addr = htonl(prefix);
					printf("withdrawn %s/%u (post-clean)\n",inet_ntoa(addr),plen);
				}
				#endif

				/* cleanup prefix: zero-ing unused bits */
				if ( plen == 0 )
					prefix = 0;
				else
					prefix &= ( 0xffffffff ^ ( ( 1 << ( 32 - plen ) ) - 1 ) );

				#ifdef DEBUG
				{
					struct in_addr addr;
					addr.s_addr = htonl(prefix);
					printf("withdrawn %s/%u (post-clean)\n",inet_ntoa(addr),plen);
				}
				#endif

				p_dump_add_withdrawn4(peer,id,ts.tv_sec,prefix,plen);
				peer[id].ucount++;
			}

			alen = *(uint16_t *) (ibuf + pos);
			alen = ntohs(alen);

			#ifdef DEBUG
			printf("announce size: %u\n",alen);
			#endif

			pos+=2;

			{
				uint16_t attrpos = 0;
				struct {
					uint8_t  flags;
					uint16_t pos;
					uint16_t len;
				} a[256];

				/* set all positions to 0xFFFF by default */
				{
					int i;
					for (i=0; i<256; i++)
						a[i].pos = 0xffff;
				}

				while(alen-attrpos>0)
				{
					uint8_t flags = *(uint8_t*) (ibuf+pos+attrpos);
					uint8_t code  = *(uint8_t*) (ibuf+pos+attrpos+1);
					uint16_t codelen;

					a[code].flags = flags;

					attrpos+=2;

					if ( flags & 0x10 )
					{
						uint16_t clen = *(uint16_t*)(ibuf+pos+attrpos);
						codelen = ntohs(clen);
						attrpos+=2;
					}
					else
					{
						codelen = *(uint8_t*)(ibuf+pos+attrpos);
						attrpos++;
					}

					a[code].pos = attrpos;
					a[code].len = codelen;
					attrpos+=codelen;

					#ifdef DEBUG

					printf("code: at offset %u of length %u is %u (%s) (",
						a[code].pos, a[code].len, code, bgp_path_attribute[code]);

					if ( flags & 0x80 ) { printf(" optional");   } else { printf(" well-known"); }
					if ( flags & 0x40 ) { printf(" transitive"); } else { printf(" non-transitive"); }
					if ( flags & 0x20 ) { printf(" partial");    } else { printf(" complete"); }
					if ( flags & 0x10 ) { printf(" 2octet");     } else { printf(" 1octet"); }
					printf(")\n");

					p_tools_dump(bgp_path_attribute[code], ibuf+pos+a[code].pos, a[code].len);

					#endif

				}

				if ( a[BGP_ATTR_ORIGIN].pos != 0xffff && config.export & EXPORT_ORIGIN )
				{
					uint16_t off     = a[BGP_ATTR_ORIGIN].pos;
					uint16_t codelen = a[BGP_ATTR_ORIGIN].len;

					if ( codelen == 1 )
					{
						origin = *(uint8_t*) (ibuf+pos+off);
						#ifdef DEBUG
						printf("ORIGIN: %u (%s)\n", origin, bgp_origin[origin]);
						#endif
					}
				}
				if ( a[BGP_ATTR_AS_PATH].pos != 0xffff && config.export & EXPORT_ASPATH )
				{
					uint16_t off     = a[BGP_ATTR_AS_PATH].pos;
					uint16_t codelen = a[BGP_ATTR_AS_PATH].len;

					uint8_t  aspath_type = *(uint8_t*) (ibuf+pos+off);
					uint8_t  aspath_len  = *(uint8_t*) (ibuf+pos+off+1);

					if ( codelen == 0 )
					{
						#ifdef DEBUG
						printf("Empty ASPATH (iBGP)\n");
						aspathlen = 0;
						#endif
					}
					else if ( aspath_type == 1 )
					{
						#ifdef DEBUG
						printf("AS_SET %u\n",aspath_len);
						#endif
					}
					else if ( aspath_type == 2 )
					{
						#ifdef DEBUG
						printf("AS_PATH %u\n",aspath_len);
						if ( peer[id].as4 == 1 )
						{
							int b;
							for(b=0; b<aspath_len; b++)
							{
								uint32_t toto = *(uint32_t*)(ibuf+pos+off+2+(b*4));
								printf("%5u ",ntohl(toto));
							}
							printf("\n");
						}
						else
						{
							int b;
							for(b=0; b<aspath_len; b++)
							{
								uint32_t toto = *(uint16_t*)(ibuf+pos+off+2+(b*2));
								printf("%5u ",ntohs(toto));
							}
							printf("\n");
						}
						#endif
						aspath = (void *)(ibuf+pos+off+2);
						aspathlen = aspath_len;
					}
					else
					{
						#ifdef DEBUG
						printf("error in aspath code, type %u unknown (len %u)\n",aspath_type,aspath_len);
						#endif

						snprintf(logline, sizeof(logline), "%s error in aspath code\n",
							peer[id].af == 4 ? p_tools_ip4str(id, &peer[id].ip4) : p_tools_ip6str(id, &peer[id].ip6) );
						p_log_add((time_t)ts.tv_sec, logline);
						peer[id].status = 0;
						return;
					}
				}

				if ( a[BGP_ATTR_COMMUNITY].pos != 0xffff && config.export & EXPORT_COMMUNITY )
				{
					uint16_t off = a[BGP_ATTR_COMMUNITY].pos;
					uint16_t codelen = a[BGP_ATTR_COMMUNITY].len;

					if ( (codelen % 4) != 0 )
					{
						snprintf(logline, sizeof(logline), "%s error in community length\n",
							peer[id].af == 4 ? p_tools_ip4str(id, &peer[id].ip4) : p_tools_ip6str(id, &peer[id].ip6) );
						p_log_add((time_t)ts.tv_sec, logline);
						peer[id].status = 0;
						return;
					}

					community = (void *) (ibuf+pos+off);
					communitylen = codelen / 4;
				}

				if ( a[BGP_ATTR_EXTCOMMUNITY].pos != 0xffff && config.export & EXPORT_EXTCOMMUNITY )
				{
					#ifdef DEBUG
					printf("Extended community not yet implemented\n");
					#endif
				}

				if ( a[BGP_ATTR_MP_REACH_NLRI].pos != 0xffff )
				{
					uint16_t off     = a[BGP_ATTR_MP_REACH_NLRI].pos;
					uint16_t codelen = a[BGP_ATTR_MP_REACH_NLRI].len;
					int         i = 0;
					uint16_t  afi = ntohs(*(uint16_t*) (ibuf+pos+off));
					uint8_t  safi = *(uint8_t*) (ibuf+pos+off+2);
					uint8_t nhlen = *(uint8_t*) (ibuf+pos+off+3);

					i += 4 + nhlen + 1;

					if ( afi == 2 && safi == 1) /* IPv6 Unicast */
					{
						while(i<codelen)
						{
							uint8_t plen = *(uint8_t*) (ibuf+pos+off+i);
							uint8_t prefix6[16];
							uint8_t blen  = ( plen % 8 ? plen / 8 + 1: plen / 8 );

							i++;


							memset(prefix6, 0, 16);
							memcpy(prefix6, ibuf+pos+off+i, blen);

							i+= blen;

							/* cleanup prefix: zero-ing unused bits */
							if ( plen % 8 )
								prefix6[blen-1] = prefix6[blen-1] & ( 0xff - ((1<<(8-(plen%8)))-1) );

							#ifdef DEBUG
							{
								char v6[40];
								struct in6_addr in6;

								sprintf(v6,"%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
									prefix6[0],  prefix6[1],  prefix6[2],  prefix6[3],
									prefix6[4],  prefix6[5],  prefix6[6],  prefix6[7],
									prefix6[8],  prefix6[9],  prefix6[10], prefix6[11],
									prefix6[12], prefix6[13], prefix6[14], prefix6[15] );

								inet_pton(AF_INET6, v6, &in6);

								printf("IPv6 prefix = %s/%u\n", p_tools_ip6str(id, &in6), plen);
							}
							#endif

							p_dump_add_announce6(
								peer,
								id,
								ts.tv_sec,
								prefix6,
								plen,
								origin,
								aspath,
								aspathlen,
								community,
								communitylen,
								extcommunity,
								extcommunitylen );
							peer[id].ucount++;
						}

					}
					#ifdef DEBUG
					else
					{
						printf("error MP_REACH_NLRI, unsupported address family %u, subsequent address family %u\n",afi,safi);
					}
					#endif
				}

				if ( a[BGP_ATTR_MP_UNREACH_NLRI].pos != 0xffff )
				{
					uint16_t off     = a[BGP_ATTR_MP_UNREACH_NLRI].pos;
					uint16_t codelen = a[BGP_ATTR_MP_UNREACH_NLRI].len;
					int         i = 0;
					uint16_t  afi = ntohs(*(uint16_t*) (ibuf+pos+off));
					uint8_t  safi = *(uint8_t*) (ibuf+pos+off+2);

					i += 3;

					if ( afi == 2 && safi == 1) /* IPv6 Unicast */
					{
						while(i<codelen)
						{
							uint8_t plen = *(uint8_t*) (ibuf+pos+off+i);
							uint8_t prefix6[16];
							uint8_t blen  = ( plen % 8 ? plen / 8 + 1: plen / 8 );

							i++;


							memset(prefix6, 0, 16);
							memcpy(prefix6, ibuf+pos+off+i, blen);

							i+= blen;

							/* cleanup prefix: zero-ing unused bits */
							if ( plen % 8 )
								prefix6[blen-1] = prefix6[blen-1] & ( 0xff - ((1<<(8-(plen%8)))-1) );

							#ifdef DEBUG
							{
								char v6[40];
								struct in6_addr in6;

								sprintf(v6,"%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
									prefix6[0],  prefix6[1],  prefix6[2],  prefix6[3],
									prefix6[4],  prefix6[5],  prefix6[6],  prefix6[7],
									prefix6[8],  prefix6[9],  prefix6[10], prefix6[11],
									prefix6[12], prefix6[13], prefix6[14], prefix6[15] );

								inet_pton(AF_INET6, v6, &in6);

								printf("IPv6 prefix = %s/%u\n", p_tools_ip6str(id, &in6), plen);
							}
							#endif

							p_dump_add_withdrawn6(
								peer,
								id,
								ts.tv_sec,
								prefix6,
								plen );
							peer[id].ucount++;
						}

					}
					#ifdef DEBUG
					else
					{
						printf("error MP_REACH_NLRI, unsupported address family %u, subsequent address family %u\n",afi,safi);
					}
					#endif
				}
			}

			pos+=alen;

			while((htons(header->len) - pos))
			{
				uint8_t  plen   = *(uint8_t *)  (ibuf+pos);
				uint32_t prefix = 0;
				int blen = 0;
				pos++;

				if ( plen > 24 )
				{
					uint32_t o = *(uint8_t *) (ibuf+pos+3);
					prefix += o << 0;
					blen++;
				}
				if ( plen > 16 )
				{
					uint32_t o = *(uint8_t *) (ibuf+pos+2);
					prefix += o << 8;
					blen++;
				}
				if ( plen > 8 )
				{
					uint32_t o = *(uint8_t *) (ibuf+pos+1);
					prefix += o << 16;
					blen++;
				}
				if ( plen > 0 )
				{
					uint32_t o = *(uint8_t *) (ibuf+pos);
					prefix += o << 24;
					blen++;
				}
				pos+=blen;

				#ifdef DEBUG
				{
					struct in_addr addr;
					addr.s_addr = htonl(prefix);
					printf("announce %s/%u (pre-clean)\n",inet_ntoa(addr),plen);
				}
				#endif

				/* cleanup prefix: zero-ing unused bits */
				if ( plen == 0 )
					prefix = 0;
				else
					prefix &= ( 0xffffffff ^ ( ( 1 << ( 32 - plen ) ) - 1 ) );

				#ifdef DEBUG
				{
					struct in_addr addr;
					addr.s_addr = htonl(prefix);
					printf("announce %s/%u (post-clean)\n",inet_ntoa(addr),plen);
				}
				#endif

				p_dump_add_announce4(peer,id,ts.tv_sec,prefix,plen,origin,
					aspath,       aspathlen,
					community,    communitylen,
					extcommunity, extcommunitylen );

				peer[id].ucount++;
			}

		}
		else if ( header->type == BGP_ERROR )
		{
			/*  bgp error msg */
			struct bgp_error *error;

			error = (struct bgp_error*) (ibuf + pos);

			#ifdef DEBUG
			printf("error code : %i/%i\n",error->code,error->subcode);
			#endif

			pos += BGP_ERROR_LEN;

			snprintf(logline, sizeof(logline), "%s notification received code %u/%u\n",
				peer[id].af == 4 ? p_tools_ip4str(id, &peer[id].ip4) : p_tools_ip6str(id, &peer[id].ip6),
				error->code,error->subcode);
			p_log_add((time_t)ts.tv_sec, logline);
			peer[id].status = 0;
		}
		else if ( header->type == BGP_KEEPALIVE && peer[id].status == 2 )
		{
			/* keepalive packet */
			peer[id].rts = ts.tv_sec;
			p_dump_add_keepalive(peer, id, ts.tv_sec);
			#ifdef DEBUG
			printf("received keepalive\n");
			#endif
		}
		else
		{
			#ifdef DEBUG
			printf("invalid header type %u\n",header->type);
			#endif

			snprintf(logline, sizeof(logline), "%s invalid message type\n",
				peer[id].af == 4 ? p_tools_ip4str(id, &peer[id].ip4) : p_tools_ip6str(id, &peer[id].ip6) );
			p_log_add((time_t)ts.tv_sec, logline);
			peer[id].status = 0;
			return;
		}

		if ( htons(header->len) != pos )
		{
			#ifdef DEBUG
			printf("something went wrong with the packet size\n");
			#endif

			snprintf(logline, sizeof(logline), "%s error in packet size\n",
				peer[id].af == 4 ? p_tools_ip4str(id, &peer[id].ip4) : p_tools_ip6str(id, &peer[id].ip6) );
			p_log_add((time_t)ts.tv_sec, logline);
			peer[id].status = 0;
			return;
		}

		if ( peer[id].ilen <= htons(header->len) )
		{
			peer[id].ilen = 0;
			return;
		}

		#ifdef DEBUG
		printf("still got datas!\n");
		#endif

		memmove(ibuf, ibuf+htons(header->len), peer[id].ilen-htons(header->len));
		peer[id].ilen -= pos;
	}
}

/* send() */
void p_main_peer_send(int id, char *obuf)
{
	/* sending datas */
	if ( peer[id].olen > 0 )
	{
		int slen = 0;

		#ifdef DEBUG
		printf("something to send\n");
		#endif

		slen = send(peer[id].sock, obuf, peer[id].olen, 0);

		if ( slen == peer[id].olen )
		{
			#ifdef DEBUG
			printf("send ok\n");
			#endif
			peer[id].olen = 0;
		}
		else if ( slen == 0 )
		{
			#ifdef DEBUG
			printf("couldnt send anything\n");
			#endif
			peer[id].olen = -1;
		}
		else if ( slen == -1 )
		{
			#ifdef DEBUG
			printf("failed to send()\n");
			#endif
			peer[id].olen = -1;
		}
		else if ( slen < peer[id].olen )
		{
			#ifdef DEBUG
			printf("cound not send all\n");
			#endif
			memmove(obuf, obuf+slen, peer[id].olen-slen);
			peer[id].olen -= slen;
		}
		else
		{
			#ifdef DEBUG
			printf("impossible send() case!\n");
			#endif
			peer[id].olen = -1;
		}
	}
	return;
}

/* peer exit, thread exit */
void p_main_peer_exit(void *data, int sock)
{
	#ifdef DEBUG
	printf("dead thready with socket %i\n",sock);
	#endif

	close(sock);

	free(data);

	pthread_exit(NULL);
}

/*  syntax */
void p_main_syntax(char *prog)
{
	printf("syntax: %s <configuration file>\n",prog);
}

/* kill -HUP for config reload */
void p_main_sighup(int sig)
{
	if ( p_config_load((struct config_t*)&config,(struct peer_t*)peer, (time_t)ts.tv_sec) == -1 )
	{
		#ifdef DEBUG
		printf("failed to reload config!\n");
		#else
		p_log_add((time_t)ts.tv_sec, "failed to reload configuration\n");
		#endif
		exit(1);
	}

	if ( p_socket_start((struct config_t*)&config, (struct peer_t*)peer) == -1 )
	{
		p_log_add((time_t)ts.tv_sec, "socket error, aborting\n");
		exit(1);
	}

	p_log_add((time_t)ts.tv_sec, "configuration reloaded\n");
	signal(sig,p_main_sighup);
}

int mydaemon(int nochdir, int noclose)
{
	int fd;

	/* set new uid/gid */
	if ( setgid(config.gid) == -1 )
	{
		#ifdef DEBUG
		printf("failed to set setgid()\n");
		#else
		p_log_add((time_t)ts.tv_sec, "failed to set setgid()\n");
		#endif
		return (-1);
	}

	if ( setegid(config.gid) == -1 )
	{
		#ifdef DEBUG
		printf("failed to set setegid()\n");
		#else
		p_log_add((time_t)ts.tv_sec, "failed to set setegid()\n");
		#endif
		return (-1);
	}

	if ( setuid(config.uid) == -1 )
	{
		#ifdef DEBUG
		printf("failed to set setuid()\n");
		#else
		p_log_add((time_t)ts.tv_sec, "failed to set setuid()\n");
		#endif
		return (-1);
	}

	if ( seteuid(config.uid) == -1 )
	{
		#ifdef DEBUG
		printf("failed to set seteuid()\n");
		#else
		p_log_add((time_t)ts.tv_sec, "failed to set seteuid()\n");
		#endif
		return (-1);
	}

	switch (fork()) {
	case -1:
		return (-1);
	case 0:
		break;
	default:
		_exit(0);
	}

	if (setsid() == -1)
		return (-1);

	if ( !nochdir && chdir("/") == -1 )
	{
		#ifdef DEBUG
		printf("failed to chdir to /\n");
		#else
		p_log_add((time_t)ts.tv_sec, "failed to chdir to /\n");
		#endif
		return (-1);
	}

	if ( !noclose && (fd = open("/dev/null", O_RDWR, 0)) != -1)
	{
		(void)dup2(fd, STDIN_FILENO);
		(void)dup2(fd, STDOUT_FILENO);
		(void)dup2(fd, STDERR_FILENO);
		if (fd > 2)
			(void)close(fd);
	}
	return (0);
}

int mychown(char *path, uid_t uid, gid_t gid, int depth)
{
	DIR *dir;
	struct dirent *item;

	if ( depth > 1 )
		return (0);

	if ( ( dir = opendir(path) ) == NULL )
		return (-1);

	while((item = readdir(dir))!=NULL)
	{
		char newpath[PATH_MAX];
		struct stat st;

		if ( item->d_name[0] == '.' )
			continue;

		sprintf(newpath, "%s/%s", path, item->d_name);

		if ( lstat(newpath, &st) == 0 )
		{
			if ( st.st_uid != uid || st.st_gid != gid )
			{
				if ( chown(newpath, uid, gid) != 0 )
				{
					printf("ERROR: Failed to chown('%s', %i, %i)\n",
						newpath, uid, gid);
					return (-1);
				}
			}

			if ( S_ISDIR(st.st_mode) ) 
				mychown(newpath, uid, gid, depth+1);
		}
	}
	
	closedir(dir);

	return (0);
}
