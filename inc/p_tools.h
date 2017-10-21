int p_tools_ip4zero(struct in_addr *ip);
int p_tools_ip6zero(struct in6_addr *ip);

int p_tools_sameip4(struct in_addr *ip1, struct in_addr *ip2);
int p_tools_sameip6(struct in6_addr *ip1, struct in6_addr *ip2);

char *p_tools_ip4str(int peerid, struct in_addr *ip);
char *p_tools_ip6str(int peerid, struct in6_addr *ip);

void p_tools_dump(const char *desc, char *data, int len);

void p_tools_humantime(char *line, size_t len, time_t ts);
