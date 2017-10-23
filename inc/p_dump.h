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


void p_dump_open_file     (struct peer_t *peer, int id, uint64_t ts);
void p_dump_add_open      (struct peer_t *peer, int id, uint64_t ts);
void p_dump_add_close     (struct peer_t *peer, int id, uint64_t ts);
void p_dump_add_keepalive (struct peer_t *peer, int id, uint64_t ts);
void p_dump_add_header4   (struct peer_t *peer, int id, uint64_t ts);
void p_dump_add_header6   (struct peer_t *peer, int id, uint64_t ts);
void p_dump_add_footer    (struct peer_t *peer, int id, uint64_t ts);
void p_dump_check_file    (struct peer_t *peer, int id, uint64_t ts);
void p_dump_close_file    (struct peer_t *peer, int id);

void p_dump_add_withdrawn4 (struct peer_t *peer, int id, uint64_t ts,
                           uint32_t prefix, uint8_t mask);
void p_dump_add_withdrawn6 (struct peer_t *peer, int id, uint64_t ts,
                           uint8_t prefix[16], uint8_t mask);

void p_dump_add_announce4 (struct peer_t *peer, int id, uint64_t ts,
                           uint32_t prefix,      uint8_t mask,
						   uint8_t origin,
                           void *aspath,         uint16_t aspathlen,
                           void *community,      uint16_t communitylen,
                           void *extcommunity4,  uint16_t extcommunitylen4,
                           void *largecommunity, uint16_t largecommunitylen );

void p_dump_add_announce6 (struct peer_t *peer, int id, uint64_t ts,
                           uint8_t prefix[16],   uint8_t mask,
						   uint8_t origin,
                           void *aspath,         uint16_t aspathlen,
                           void *community,      uint16_t communitylen,
                           void *extcommunity6,  uint16_t extcommunitylen6,
                           void *largecommunity, uint16_t largecommunitylen );
