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

#include <p_defs.h>
#include <p_socket.h>
#include <p_tools.h>


/*  init the listening socket */
int p_socket_start(struct config_t *config, struct peer_t *peer)
{
	#ifdef LINUX
	int peerid;
	#endif
	struct timeval timeout;

	timeout.tv_sec = 0;
	timeout.tv_usec = 100000;

	if ( config->ip4.enabled )
	{
		#ifdef DEBUG
		printf("DEBUG: setting up IPv4 listening socket\n");
		#endif
	
		if ( config->ip4.sock > 0 )
		{
			#ifdef DEBUG
			printf("DEBUG: socket4 still open, closing\n");
			#endif
			close(config->ip4.sock);
		}
	
		if ( ( config->ip4.sock = socket(PF_INET, SOCK_STREAM, 0) ) == -1 )
		{
			#ifdef DEBUG
			printf("DEBUG: failed to init socket4\n");
			#endif
			return -1;
		}
	
		#ifdef DEBUG
		else { printf("DEBUG: socket() ok\n"); }
		#endif
	
		if ( setsockopt(config->ip4.sock, SOL_SOCKET, SO_REUSEADDR, "1", sizeof(int)) == -1 )
		{
			#ifdef DEBUG
			printf("DEBUG: failed to setsockopt() SO_REUSEADDR\n");
			#endif
			return -1;
		}
		#ifdef DEBUG
		else { printf("DEBUG: setsockopt() (SO_REUSEADDR) ok\n"); }
		#endif
	
		if ( ( setsockopt(config->ip4.sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) ) == -1 )
		{
			#ifdef DEBUG
			printf("DEBUG: failed to setsockopt() SO_SNDTIMEO\n");
			#endif
			return -1;
		}
		#ifdef DEBUG
		else { printf("DEBUG: setsockopt() (SO_SNDTIMEO) ok\n"); }
		#endif
	
		if ( ( setsockopt(config->ip4.sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) ) == -1 )
		{
			#ifdef DEBUG
			printf("DEBUG: failed to setsockopt() SO_RCVTIMEO\n");
			#endif
			return -1;
		}
		#ifdef DEBUG
		else { printf("DEBUG: setsockopt() (SO_RCVTIMEO) ok\n"); }
		#endif
	
		if ( fcntl(config->ip4.sock, F_SETFL, O_NONBLOCK) == -1 )
		{
			#ifdef DEBUG
			printf("DEBUG: failed to fcntl() O_NONBLOCK\n");
			#endif
			return -1;
		}
		#ifdef DEBUG
		else { printf("DEBUG: fcntl() (O_NONBLOCK) ok\n"); }
		#endif
	
		if ( bind(config->ip4.sock, (struct sockaddr*)&config->ip4.listen, sizeof(struct sockaddr_in)) == -1 )
		{
			#ifdef DEBUG
			printf("DEBUG: failed to bind socket\n");
			#endif
			return -1;
		}
		#ifdef DEBUG
		else { printf("DEBUG: bind() ok\n"); }
		#endif
	}

	if ( config->ip6.enabled )
	{
		#ifdef DEBUG
		printf("DEBUG: setting up IPv6 listening socket\n");
		#endif
	
		if ( config->ip6.sock > 0 )
		{
			#ifdef DEBUG
			printf("DEBUG: socket4 still open, closing\n");
			#endif
			close(config->ip6.sock);
		}
	
		if ( ( config->ip6.sock = socket(PF_INET6, SOCK_STREAM, 0) ) == -1 )
		{
			#ifdef DEBUG
			printf("DEBUG: failed to init socket4\n");
			#endif
			return -1;
		}
		#ifdef DEBUG
		else { printf("DEBUG: socket() ok\n"); }
		#endif

		if ( setsockopt(config->ip6.sock, IPPROTO_IPV6, IPV6_V6ONLY, "1", sizeof(int)) == -1 )
		{
			#ifdef DEBUG
			printf("DEBUG: failed to setsockopt() IPV6_V6ONLY\n");
			#endif
			return -1;
		}
		#ifdef DEBUG
		else { printf("DEBUG: setsockopt() (IPV6_V6ONLY) ok\n"); }
		#endif
	
		if ( setsockopt(config->ip6.sock, SOL_SOCKET, SO_REUSEADDR, "1", sizeof(int)) == -1 )
		{
			#ifdef DEBUG
			printf("DEBUG: failed to setsockopt() SO_REUSEADDR\n");
			#endif
			return -1;
		}
		#ifdef DEBUG
		else { printf("DEBUG: setsockopt() (SO_REUSEADDR) ok\n"); }
		#endif
	
		if ( ( setsockopt(config->ip6.sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) ) == -1 )
		{
			#ifdef DEBUG
			printf("DEBUG: failed to setsockopt() SO_SNDTIMEO\n");
			#endif
			return -1;
		}
		#ifdef DEBUG
		else { printf("DEBUG: setsockopt() (SO_SNDTIMEO) ok\n"); }
		#endif
	
		if ( ( setsockopt(config->ip6.sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) ) == -1 )
		{
			#ifdef DEBUG
			printf("DEBUG: failed to setsockopt() SO_RCVTIMEO\n");
			#endif
			return -1;
		}
		#ifdef DEBUG
		else { printf("DEBUG: setsockopt() (SO_RCVTIMEO) ok\n"); }
		#endif
	
		if ( fcntl(config->ip6.sock, F_SETFL, O_NONBLOCK) == -1 )
		{
			#ifdef DEBUG
			printf("DEBUG: failed to fcntl() O_NONBLOCK\n");
			#endif
			return -1;
		}
		#ifdef DEBUG
		else { printf("DEBUG: fcntl() (O_NONBLOCK) ok\n"); }
		#endif
	
		if ( bind(config->ip6.sock, (struct sockaddr*)&config->ip6.listen, sizeof(struct sockaddr_in6)) == -1 )
		{
			#ifdef DEBUG
			printf("DEBUG: failed to bind socket\n");
			#endif
			return -1;
		}
		#ifdef DEBUG
		else { printf("DEBUG: bind() ok\n"); }
		#endif
	}


	// TCP MD5 currently only supported on Linux
	#ifdef LINUX
	for(peerid=0; peerid<MAX_PEERS; peerid++)
	{
		if ( peer[peerid].key[0] != '\0' )
		{
			int r;
			struct tcp_md5sig md5;
			memset(&md5, 0, sizeof(md5));

			if ( peer[peerid].af == 4 )
			{
				struct sockaddr_in  paddr;
				memset(&paddr, 0, sizeof(paddr));
				paddr.sin_family = AF_INET;
				memcpy(&paddr.sin_addr,   &peer[peerid].ip4, sizeof(peer[peerid].ip4));
				memcpy(&md5.tcpm_addr, &paddr, sizeof(paddr));

			}
			else if ( peer[peerid].af == 6 )
			{
				struct sockaddr_in6 paddr6;
				memset(&paddr6, 0, sizeof(paddr6));
				paddr6.sin6_family = AF_INET6;
				memcpy(&paddr6.sin6_addr,   &peer[peerid].ip6, sizeof(peer[peerid].ip6));
				memcpy(&md5.tcpm_addr, &paddr6, sizeof(paddr6));
			}

			memcpy(&md5.tcpm_key, peer[peerid].key, strlen(peer[peerid].key));
			md5.tcpm_keylen = strlen(peer[peerid].key);

			if ( peer[peerid].af == 4 && config->ip4.enabled )
			{

				#ifdef DEBUG
				printf("md5 key %s len %i addr %s\n",
					md5.tcpm_key,
					md5.tcpm_keylen,
					p_tools_ip4str(peerid, &peer[peerid].ip4));
				#endif

				if ( ( r = setsockopt(config->ip4.sock, IPPROTO_TCP, TCP_MD5SIG, &md5, sizeof md5)) != 0 )
				{
					#ifdef DEBUG
					printf("Activating MD5SIG failed: '%s'\n", strerror(errno));
					#endif
					return -1;
				}
			}
			else if ( peer[peerid].af == 6 && config->ip6.enabled )
			{

				#ifdef DEBUG
				printf("md5 key %s len %i addr %s\n",
					md5.tcpm_key,
					md5.tcpm_keylen,
					p_tools_ip6str(peerid, &peer[peerid].ip6));
				#endif

				if ( ( r = setsockopt(config->ip6.sock, IPPROTO_TCP, TCP_MD5SIG, &md5, sizeof md5)) != 0 )
				{
					#ifdef DEBUG
					printf("Activating MD5SIG failed: '%s'\n", strerror(errno));
					#endif
					return -1;
				}
			}
		}
	}
	#endif

	if ( config->ip4.enabled )
	{
		if ( listen(config->ip4.sock, 10) == -1 )
		{
			#ifdef DEBUG
			printf("DEBUG: failed to listen() socket\n");
			#endif
			return -1;
		}
		#ifdef DEBUG
		else { printf("DEBUG: listen() ok\n"); }
		#endif
	}

	if ( config->ip6.enabled )
	{

		if ( listen(config->ip6.sock, 10) == -1 )
		{
			#ifdef DEBUG
			printf("DEBUG: failed to listen() socket\n");
			#endif
			return -1;
		}
		#ifdef DEBUG
		else { printf("DEBUG: listen() ok\n"); }
		#endif
	}

	return 0;
}

/* check for new peers */
int p_socket_accept(struct config_t *config)
{
	int sock;
	struct sockaddr_in sockaddr;
	struct sockaddr_in6 sockaddr6;
	unsigned int addrlen = sizeof(sockaddr);
	unsigned int addrlen6 = sizeof(sockaddr6);

	if ( ( sock = accept(config->ip4.sock, (struct sockaddr*)&sockaddr, &addrlen) ) == -1 )
	{
		if ( ( sock = accept(config->ip6.sock, (struct sockaddr*)&sockaddr6, &addrlen6) ) == -1 )
		{
			return -1;
		}
	}
	return sock;
}
