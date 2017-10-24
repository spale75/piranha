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

enum PTOA_MODE { PTOA_NONE, PTOA_HUMAN, PTOA_JSON, PTOA_MACHINE };


int main(int argc, char *argv[]);
// void mytime(time_t ts);
void syntax(char *prog);
void print_origin(int mode, uint8_t origin);
void print_aspath(int mode, struct dump_announce_aspath *aspath, uint8_t len);
void print_community(int mode, struct dump_announce_community *community, uint16_t len);
void print_extcommunity4(int mode, struct dump_announce_extcommunity4 *com, uint16_t len);
void print_extcommunity6(int mode, struct dump_announce_extcommunity6 *com, uint16_t len);
void print_largecommunity(int mode, struct dump_announce_largecommunity *com, uint16_t len);
