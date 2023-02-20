#include<string.h>
#include"stdtype.h"
#include"tokenutil.h"


bool StrCmp::operator()(const Char* str1, const Char* str2) const {
    return strcmp(str1, str2);
}

bool TransCmp::operator()(const IpInfo* ip1, const IpInfo* ip2) const {
    Int32 ret = 0;
    
    if (ip1->m_port != ip2->m_port) {
        return ip1->m_port < ip2->m_port;
    } else {
        ret = strncmp(ip1->m_ip, ip2->m_ip, MAX_IP_SIZE);
        return 0 > ret;
    }
}

bool TokenCmp::operator()(const Token* token1, const Token* token2) const {
    Int32 ret = 0;

    ret = TokenUtil::cmpToken(token1, token2);
    return 0 > ret;
}

