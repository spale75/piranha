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
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


#include <p_defs.h>
#include <p_log.h>
#include <p_tools.h>

/* update pid file */
void p_log_pid()
{
	FILE *fh;
	char pidstr[10];
	pid_t pid = getpid();

	if ( ( fh = fopen(PIDFILE,"w") ) == NULL ) { return; }

	snprintf(pidstr, sizeof(pidstr), "%i\n",pid);

	fwrite(pidstr, strlen(pidstr), 1, fh);

	fclose(fh);
}

/* add text to logfile */
void p_log_add(time_t mytime, char *line)
{
	struct tm *tm;
	char timestr[40];
	FILE *fh;

	tm = gmtime((time_t*)&mytime);

	strftime(timestr, sizeof(timestr), "[%Y-%m-%d %H:%M:%S] " , tm);

	if ( ( fh = fopen(LOGFILE,"a") ) == NULL ) { return; }

	fwrite(timestr, strlen(timestr), 1, fh);
	fwrite(line, strlen(line), 1, fh);

	fclose(fh);
}

/* convert time_t in weeks,days,hours,mins,sec format */
void p_log_easytime(time_t mytime, char *timestr, int timestrlen)
{
	uint16_t s = 0;
	uint16_t m = 0;
	uint16_t h = 0;
	uint16_t d = 0;
	uint16_t w = 0;

	s = mytime % 60;
	mytime = ( mytime - s ) / 60;

	m = mytime % 60;
	mytime = ( mytime - m ) / 60;

	h = mytime % 24;
	mytime = ( mytime - h ) / 24;

	d = mytime % 7;
	mytime = ( mytime - d ) / 7;

	w = mytime;

	if ( w > 0 )      { snprintf(timestr, timestrlen, "%uw%ud", w, d); }
	else if ( d > 0 ) { snprintf(timestr, timestrlen, "%ud%uh", d, h); }
	else if ( h > 0 ) { snprintf(timestr, timestrlen, "%uh%um", h, m); }
	else              { snprintf(timestr, timestrlen, "%um%us", m, s); }
}

/* update status file */
void p_log_status(struct config_t *config, time_t mytime)
{
	char data[(MAX_PEERS*64)+256];
	int doff = 0;
	int a;
	FILE *fh;
	static char *bgp_status[] = { "down", "temp", "up", };

	snprintf(data,      sizeof(data),      "/----------------------------------------------------------------------------------------------------\\\n");
	doff = strlen(data);
	snprintf(data+doff, sizeof(data)-doff, "| neighbor                                      asn        recv       sent  updates  status  up/down |\n");
	doff = strlen(data);
	snprintf(data+doff, sizeof(data)-doff, "|----------------------------------------------------------------------------------------------------|\n");
	doff = strlen(data);

	for(a=0; a<MAX_PEERS; a++)
	{
		if ( config->peer[a].allow )
		{
			char timestr[1024];

			p_log_easytime(mytime - config->peer[a].cts, timestr, sizeof(timestr));

			if ( config->peer[a].status != 2 && config->peer[a].ucount ) { config->peer[a].ucount = 0; }

			snprintf(data+doff, sizeof(data)-doff, "| %-39s %10u %10u %10u  %7u %7s %8s |\n",
				config->peer[a].af == 4 ? p_tools_ip4str(a, &config->peer[a].ip4) : p_tools_ip6str(a, &config->peer[a].ip6),
				config->peer[a].as,
				config->peer[a].rmsg,
				config->peer[a].smsg,
				config->peer[a].ucount,
				bgp_status[config->peer[a].status],
				timestr );

			doff = strlen(data);
		}
	}

	snprintf(data+doff, sizeof(data)-doff, "\\----------------------------------------------------------------------------------------------------/\n");
	if ( ( fh = fopen(STATUSTEMP,"w") ) == NULL ) { return; }
	fwrite(data, strlen(data), 1, fh);
	fclose(fh);
	rename(STATUSTEMP, STATUSFILE);
}
