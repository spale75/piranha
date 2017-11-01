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
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>

#include <p_defs.h>
#include <p_config.h>
#include <p_tools.h>

/* reading configuration file */

int p_config_load(struct config_t *config, struct peer_t *peer, uint32_t mytime)
{
	FILE *fd;
	char line[128];

	/* cleaning 'newallow' */
	{
		int a;
		for(a=0; a<MAX_PEERS; a++)
		{
			peer[a].newallow = 0;
		}
	}

	config->uid = -1;
	config->gid = -1;

	if ( ( fd = fopen(config->file, "r") ) == NULL )
	{
		printf("error: failed to read %s\n",config->file);
		return -1;
	}

	while(fgets(line, sizeof(line), fd))
	{
		char *s = line;
		if ( strncmp(s, "#", 1) == 0 ) { continue; }
		s = strtok(s, " ");

		if ( !strcmp(s,"bgp_router_id"))
		{
			s = strtok(NULL, " ");
			if ( s != NULL && strlen(s) > 0 && strlen(s) <= 16 )
			{
				config->routerid = ntohl(inet_addr(s));
				#ifdef DEBUG
				printf("DEBUG: config bgp_router_id %s",s);
				#endif
			}
		}
		else if ( !strcmp(s,"user"))
		{
			s = strtok(NULL, " ");
			if ( s != NULL && strlen(s) > 0 && strlen(s) <= 50 )
			{
				struct passwd *mypwd;
				CHOMP(s);
				mypwd = getpwnam(s);
				if ( mypwd == NULL )
				{
					config->uid = -1;
					config->gid = -1;
				}
				else
				{
					config->uid = mypwd->pw_uid;
					config->gid = mypwd->pw_gid;
				}
				#ifdef DEBUG
				printf("DEBUG: config user %i/%i (uid/gid) %s\n",config->uid,config->gid,s);
				#endif
			}
		}
		else if ( !strcmp(s,"local_as"))
		{
			s = strtok(NULL, " ");
			if ( s != NULL && strlen(s) > 0 && strlen(s) <=  6 )
			{
				config->as = atol(s);
				#ifdef DEBUG
				printf("DEBUG: config local_as %s",s);
				#endif
			}
		}
		else if ( !strcmp(s,"local_port4"))
		{
			s = strtok(NULL, " ");
			if ( s != NULL && strlen(s) > 0 && strlen(s) <= 6 )
			{
				config->ip4.listen.sin_family = AF_INET;
				config->ip4.listen.sin_port = htons(atoi(s));
				#ifdef DEBUG
				printf("DEBUG: config local_port4 %s",s);
				#endif
			}
		}
		else if ( !strcmp(s,"local_port6"))
		{
			s = strtok(NULL, " ");
			if ( s != NULL && strlen(s) > 0 && strlen(s) <= 6 )
			{
				config->ip6.listen.sin6_family = AF_INET6;
				config->ip6.listen.sin6_port = htons(atoi(s));
				#ifdef DEBUG
				printf("DEBUG: config local_port6 %s",s);
				#endif
			}
		}
		else if ( !strcmp(s, "local_ip4"))
		{
			s = strtok(NULL, " ");
			if ( s != NULL && strlen(s) > 0 && strlen(s) <= 15 )
			{
				CHOMP(s);
				if ( inet_pton(AF_INET, s, &config->ip4.listen.sin_addr) == 1 )
				{
					config->ip4.enabled=1;
					#ifdef DEBUG
					printf("DEBUG: config local_ipv4 %s\n", s);
					#endif
				}
			}
		}
		else if ( !strcmp(s, "local_ip6"))
		{
			s = strtok(NULL, " ");
			if ( s != NULL && strlen(s) > 0 && strlen(s) <= 47 )
			{
				CHOMP(s);
				if ( inet_pton(AF_INET6, s, &config->ip6.listen.sin6_addr) == 1 )
				{
					config->ip6.enabled=1;
					#ifdef DEBUG
					printf("DEBUG: config local_ipv6 %s\n", s);
					#endif
				}
			}
		}
		else if ( !strcmp(s,"bgp_holdtime"))
		{
			s = strtok(NULL, " ");
			if ( s != NULL && strlen(s) > 0 && strlen(s) <= 4 )
			{
				config->holdtime = atoi(s);
				#ifdef DEBUG
				printf("DEBUG: config bgp_holdtime %s",s);
				#endif
			}
		}
		else if ( !strcmp(s, "export"))
		{
			s = strtok(NULL, " ");
			CHOMP(s);
			if ( s != NULL && strlen(s) > 0 && strlen(s) < 100 )
			{
				#ifdef DEBUG
				printf("DEBUG: config export %s\n", s);
				#endif

				if ( ! strcmp(s, "origin") )
					config->export |= EXPORT_ORIGIN;
				else if ( ! strcmp(s, "aspath") )
					config->export |= EXPORT_ASPATH;
				else if ( ! strcmp(s, "community") )
					config->export |= EXPORT_COMMUNITY;
				else if ( ! strcmp(s, "extcommunity") )
					config->export |= EXPORT_EXTCOMMUNITY;
				else if ( ! strcmp(s, "largecommunity") )
					config->export |= EXPORT_LARGECOMMUNITY;
				else if ( ! strcmp(s, "nexthop") )
					config->export |= EXPORT_NEXT_HOP;
				#ifdef DEBUG
				else
					printf("DEBUG: Unknown export %s\n", s);
				#endif
			}
		}
		else if ( !strcmp(s,"neighbor"))
		{
			s = strtok(NULL, " ");
			if ( s != NULL )
			{
				struct in_addr  peer_ip4;
				struct in6_addr peer_ip6;
				uint8_t af = 0;

				if ( inet_pton(AF_INET6, s, &peer_ip6) == 1 )
				{
					af=6;
					#ifdef DEBUG
					printf("DEBUG: config neighbor IP6 %s ",
						p_tools_ip6str(MAX_PEERS, &peer_ip6));
					#endif
				}
				else if ( inet_pton(AF_INET, s, &peer_ip4) == 1 )
				{
					af=4;
					#ifdef DEBUG
					printf("DEBUG: config neighbor IP4 %s ",
						p_tools_ip4str(MAX_PEERS, &peer_ip4));
					#endif
				}
				else
				{
					#ifdef DEBUG
					printf("DEBUG: config neighbor, invalid IP '%s'\n",s);
					#endif
				}
			
				s = strtok(NULL, " ");
				if ( s != NULL && strlen(s) > 0 && strlen(s) <= 10 )
				{
					uint32_t peer_as = strtol(s, NULL, 10);
					char peer_key[MAX_KEY_LEN];
					CHOMP(s);
					#ifdef DEBUG
					printf("as %s",s);
					#endif

					s = strtok(NULL," ");
					if ( s != NULL && strlen(s) > 0 && strlen(s) < MAX_KEY_LEN )
					{
						int len = strlen(s);
						if ( len>=2 && s[len-2] == '\r' )
							len-=2;
						else if ( len>=1 && s[len-1] == '\n' )
							len-=1;

						s[len] = '\0';

						if ( len > 0 )
						{
							strcpy(peer_key, s);
							#ifdef DEBUG
							printf(" key %s",s);
							#endif
						}
					}
					else
						peer_key[0] = '\0';

					if ( af == 4 || af == 6 )
						p_config_add_peer(peer, af, &peer_ip4, &peer_ip6, peer_as, peer_key, mytime);

				}
				#ifdef DEBUG
				printf("\n");
				#endif
			}
		}
	}

	fclose(fd);


	/* clearning no more allowed peers  *
	 * and set session type (eBGP/iBGP) */
	{
		int a;
		for(a=0; a<MAX_PEERS; a++)
		{
			if ( peer[a].as == config->as )
				peer[a].type = BGP_TYPE_IBGP;
			else
				peer[a].type = BGP_TYPE_EBGP;

			if ( peer[a].newallow == 0 )
			{
				peer[a].status = 0;
				peer[a].allow  = 0;
			}
		}
	}

	/* check that all required values are set */

	if ( config->routerid == 0 )
	{
		printf("configuration error: no bgp router id set\n");
		return -1;
	}
	if ( config->as == 0 )
	{
		printf("configuration error: no local AS set\n");
		return -1;
	}

	if ( config->uid == -1 || config->gid == -1 )
	{
		printf("configuration error: could not find user\n");
		return -1;
	}

	return 0;
}

/* add, update of peers */
void p_config_add_peer(struct peer_t *peer, uint8_t af, struct in_addr *ip4, struct in6_addr *ip6, uint32_t as, char *key, uint32_t mytime)
{
	int a;
	if ( as == 0 )
		return;

	if ( af == 4 && p_tools_ip4zero(ip4) == 1 )
		return;

	if ( af == 6 && p_tools_ip6zero(ip6) == 1 )
		return;

	for(a = 0; a<MAX_PEERS; a++)
	{
		if ( peer[a].af == af )
		{
			if (
				( af == 4 && p_tools_sameip4(ip4, &peer[a].ip4) && peer[a].allow == 1 ) ||
				( af == 6 && p_tools_sameip6(ip6, &peer[a].ip6) && peer[a].allow == 1 )
			) {
			
				if ( peer[a].as != as )
				{
					peer[a].as     = as;
					peer[a].cts    = mytime;
					peer[a].status = 0;
				}
				if ( strcmp(peer[a].key, key) != 0 )
				{
					strcpy(peer[a].key, key);
					peer[a].cts    = mytime;
					peer[a].status = 0;
				}
				peer[a].newallow = 1;
				return;
			}
		}
	}

	for(a = 0; a<MAX_PEERS; a++)
	{
		if ( peer[a].allow == 0 )
		{
			if ( af == 4 )
				memcpy(&peer[a].ip4, ip4, sizeof(*ip4) );
			else
				memcpy(&peer[a].ip6, ip6, sizeof(*ip6) );

			peer[a].af       = af;
			peer[a].as       = as;
			peer[a].allow    = 1;
			peer[a].newallow = 1;
			peer[a].status   = 0;
			peer[a].sock     = 0;
			peer[a].cts      = mytime;
			strcpy(peer[a].key, key);
			return;
		}
	}
}
