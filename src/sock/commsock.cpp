#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<net/if.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<fcntl.h> 
#include"commsock.h"
#include"misc.h"


static const Int32 DEF_SOCK_DOMAIN[ENUM_DOMAIN_END] = {
    AF_INET,
    AF_INET6,
    AF_UNIX
};

static const Int32 DEF_SOCK_TYPE[ENUM_SOCK_TYPE_END] = {
    SOCK_STREAM,
    SOCK_DGRAM
};

static const Int32 DEF_SOCK_LEVEL[ENUM_SOCK_LEVEL_END] = {
    SOL_SOCKET,
    IPPROTO_IP,
};

static const Int32 DEF_SOCK_OPTNAME[ENUM_SOCK_OPT_NAME_END] = {
    SO_REUSEADDR, 
    SO_ERROR,
    SO_LINGER,
    SO_SNDBUF,
    SO_RCVBUF,

    /* ip opt */
    IP_ADD_MEMBERSHIP,
    IP_DROP_MEMBERSHIP,
    IP_HDRINCL,
    IP_MULTICAST_TTL,
};

Void CommSock::copyAddr(const AddrInfo* src, AddrInfo* dst) {
    if (src->m_size < MAX_ADDR_SIZE) {
        dst->m_size = src->m_size;
    } else {
        dst->m_size = MAX_ADDR_SIZE - 1;
    }
    
    memcpy(dst->m_addr, src->m_addr, dst->m_size);
}

Void CommSock::copyIP(const IpInfo* src, IpInfo* dst) {
    Int32 len = 0;

    len = strnlen(src->m_ip, MAX_IP_SIZE);
    if (len < MAX_IP_SIZE) {
        strncpy(dst->m_ip, src->m_ip, len);
        dst->m_ip[len] = DEF_NULL_CHAR;
    } else {
        dst->m_ip[0] = DEF_NULL_CHAR;
    } 
    
    dst->m_port = src->m_port;
}

Void CommSock::assignIP(const Char* ip, Int32 port, IpInfo* dst) {
    Int32 len = 0;

    len = strnlen(ip, MAX_IP_SIZE);
    if (len < MAX_IP_SIZE) {
        strncpy(dst->m_ip, ip, len);
        dst->m_ip[len] = DEF_NULL_CHAR;
    } else {
        dst->m_ip[0] = DEF_NULL_CHAR;
    } 
    
    dst->m_port = port;
}

Void CommSock::setIP(const Token* ip, Int32 port, IpInfo* dst) {
    if (0 < ip->m_size && MAX_IP_SIZE > ip->m_size) {
        strncpy(dst->m_ip, ip->m_token, ip->m_size);
        dst->m_ip[ip->m_size] = DEF_NULL_CHAR;
    } else {
        dst->m_ip[0] = DEF_NULL_CHAR;
    }

    dst->m_port = port;
}

Int32 CommSock::creatSock(EnumDomain domain, EnumSockType type,
    Int32 protocols, Int32* pFd) {
    Int32 ret = 0;

    ret = socket(DEF_SOCK_DOMAIN[domain], DEF_SOCK_TYPE[type], protocols);
    if (0 <= ret) { 
        *pFd = ret;
        
        return 0;
    } else {
        LOG_ERROR("create_sock| ret=%d| domain=%d| type=%d|"
            " protocols=%d| error=%s|", 
            ret, domain, type, protocols, ERR_MSG());

        *pFd = -1;
        return -1;
    }
}

Int32 CommSock::setSockOpt(Int32 fd, EnumSockLevel level,
    EnumSockOptName optName,
    const Void* optVal, Int32 optLen) {
    Int32 ret = 0;

    ret = setsockopt(fd, DEF_SOCK_LEVEL[level],
        DEF_SOCK_OPTNAME[optName],
        optVal, optLen);
    if (0 == ret) { 
        LOG_INFO("set_sock_opt| fd=%d| level=%d| optname=%d|"
            " size=%d| msg=ok|",
            fd, level, optName, optLen);
        
        return 0;
    } else {
        LOG_ERROR("set_sock_opt| fd=%d| level=%d| optname=%d|"
            " size=%d| error=%s|",
            fd, level, optName, optLen, ERR_MSG());
        
        return -1;
    }
}

Int32 CommSock::getSockOpt(Int32 fd, EnumSockLevel level,
    EnumSockOptName optName,
    Void* optVal, Int32* optLen) {
    Int32 ret = 0;
    socklen_t size = 0;

    size = *optLen;
    ret = getsockopt(fd, DEF_SOCK_LEVEL[level],
        DEF_SOCK_OPTNAME[optName],
        optVal, &size);
    if (0 == ret) {
        *optLen = size;
        
        return 0;
    } else {
        LOG_ERROR("get_sock_opt| fd=%d| level=%d| optname=%d|"
            " size=%d| error=%s|",
            fd, level, optName, size, ERR_MSG());
        
        return -1;
    }
}

Int32 CommSock::setNonBlock(Int32 fd) {
    Int32 ret = 0;
    Int32 flag = 0;

    flag = fcntl(fd, F_GETFL);
    flag |= O_NONBLOCK;
    
    ret = fcntl(fd, F_SETFL, flag);
    return ret;
}

Int32 CommSock::setReuse(Int32 fd) {
    Int32 ret = 0;

    ret = setSockOpt(fd, ENUM_SOCK_LEVEL_SOL_SOCKET,
        ENUM_SOCK_OPT_REUSE_ADDR, 
        &DEF_OPT_VAL_ON, DEF_OPT_VAL_SIZE);

    return ret;
}

Int32 CommSock::setNoTimewait(Int32 fd) {
    Int32 ret = 0;
    Int32 size = 0;
    struct linger val;
	 
    val.l_onoff=1;
    val.l_linger=0;
    size = sizeof(val);
    
    ret = setSockOpt(fd, ENUM_SOCK_LEVEL_SOL_SOCKET, ENUM_SOCK_OPT_LINGER, 
        &val, size);
    
    return ret;
}

Int32 CommSock::setMulticastTtl(Int32 fd, Int32 ttl) {
    Int32 ret = 0;
    Int32 size = 0;
    
    size = sizeof(ttl); 
    ret = setSockOpt(fd, ENUM_SOCK_LEVEL_IPPROTO_IP, 
        ENUM_IP_OPT_MULTICAST_TTL, &ttl, size);
    
    return ret;
}

Bool CommSock::isValidPort(Int32 port) {
    if (0 < port && port < 0x10000) {
        return TRUE;
    } else {
        return FALSE;
    }
}

Int32 CommSock::ip2Addr(EnumDomain domain, EnumSockType type,
    const IpInfo* ip, AddrInfo* addr) {
    Int32 ret = 0;
    Bool bOk = FALSE;
    struct addrinfo* result = NULL;
    struct addrinfo hints;
    Char tmp[COMM_BUFFER_SIZE] = {0};

    bOk = isValidPort(ip->m_port);
    if (!bOk) {
        LOG_ERROR("ip_to_addr| ip=%s| port=%d| msg=port out of range|",
            ip->m_ip, ip->m_port);

        return -1;
    }
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = DEF_SOCK_DOMAIN[domain];
    hints.ai_socktype = DEF_SOCK_TYPE[type];
    hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV | AI_ADDRCONFIG;

    snprintf(tmp, COMM_BUFFER_SIZE, "%d", ip->m_port);
    ret = getaddrinfo(ip->m_ip, tmp, &hints, &result);
    if (0 == ret) {
        addr->m_size = (Int32)result->ai_addrlen;
        
        if (addr->m_size < MAX_ADDR_SIZE) {
            memcpy(addr->m_addr, result->ai_addr, addr->m_size);
        } else {
            LOG_ERROR("ip_to_addr| ip=%s| port=%d| expect_size=%d|"
                " error=exceeds max addr size|",
                ip->m_ip, ip->m_port, addr->m_size);
            
            addr->m_size = 0;
            ret = -1;
        }

        /* release resource */
        freeaddrinfo(result);
    } else {
        LOG_ERROR("ip_to_addr| ip=%s| port=%d| error=%s|",
            ip->m_ip, ip->m_port, gai_strerror(ret));

        addr->m_size = 0;
        ret = -2;
    }

    return ret;
}

Int32 CommSock::addr2IP(const AddrInfo* addr, IpInfo* ip) {
    Bool bOk = FALSE;
    Int32 ret = 0;
    Int32 flags = 0;
    Char tmp[COMM_BUFFER_SIZE] = {0};
        
    flags = NI_NUMERICHOST | NI_NUMERICSERV;
    ret = getnameinfo(( struct sockaddr*)addr->m_addr, addr->m_size,
        ip->m_ip, MAX_IP_SIZE, 
        tmp, COMM_BUFFER_SIZE, flags);
    if (0 == ret) {
        ip->m_port = atoi(tmp);
        bOk = isValidPort(ip->m_port);
        if (!bOk) {
            LOG_ERROR("addr_to_ip| ip=%s| port=%s|"
                " error=invalid port|",
                ip->m_ip, tmp);
            
            ip->m_port = -1;
            ret = -2;
        }
    } else {
        LOG_ERROR("addr_to_ip| addr_len=%d| error=%s|",
            addr->m_size, gai_strerror(ret));

        ip->m_port = -1;
        ip->m_ip[0] = '\0';
        ret = -1;
    }

    return ret;
}

Int32 CommSock::ipstr2N(EnumDomain domain, const Char* ip, 
    Byte (&addr)[MAX_ADDR_SIZE]) {
    Int32 ret = 0;

    ret = inet_pton(DEF_SOCK_DOMAIN[domain], ip, addr);
    if (1 == ret) {
        return 0;
    } else {
        LOG_ERROR("ip_pton| ret=%d| ip=%s| error=%s|",
            ret, ip, ERR_MSG());
        return -1;
    }
}

Int32 CommSock::ipn2Str(EnumDomain domain, const Void* addr,
    Char (&ip)[MAX_IP_SIZE]) {
    const Char* psz = NULL;

    psz = inet_ntop(DEF_SOCK_DOMAIN[domain], addr, ip, MAX_IP_SIZE);
    if (NULL != psz) {
        return 0;
    } else {
        LOG_ERROR("ip_ntop| error=%s|", ERR_MSG());
        return -1;
    }
}

Int32 CommSock::getLocalInfo(Int32 fd, IpInfo* local_ip,
    AddrInfo* local_addr) {
    Int32 ret = 0;
    IpInfo ipInfo;
    AddrInfo addr;
    socklen_t size = 0;

    size = MAX_ADDR_SIZE;
    ret = getsockname(fd, (struct sockaddr*)addr.m_addr, &size);
    if (0 == ret) { 
        addr.m_size = size;
        
        ret = addr2IP(&addr, &ipInfo);
    } 

    if (0 == ret) {
        if (NULL != local_addr) {
            copyAddr(&addr, local_addr);
        }

        if (NULL != local_ip) {
            copyIP(&ipInfo, local_ip);
        }
    } else {
        if (NULL != local_addr) {
            memset(local_addr, 0, sizeof(*local_addr));
        }

        if (NULL != local_ip) {
            memset(local_ip, 0, sizeof(*local_ip));
        }
    }

    return ret;
}

Int32 CommSock::getPeerInfo(Int32 fd, IpInfo* peer_ip, 
    AddrInfo* peer_addr) {
    Int32 ret = 0;
    IpInfo ipInfo;
    AddrInfo addr;
    socklen_t size = 0;

    size = MAX_ADDR_SIZE;
    ret = getpeername(fd, (struct sockaddr*)addr.m_addr, &size);
    if (0 == ret) { 
        addr.m_size = size;
        
        ret = addr2IP(&addr, &ipInfo);
    } 

    if (0 == ret) {
        if (NULL != peer_addr) {
            copyAddr(&addr, peer_addr);
        }

        if (NULL != peer_ip) {
            copyIP(&ipInfo, peer_ip);
        }
    } else {
        if (NULL != peer_addr) {
            memset(peer_addr, 0, sizeof(*peer_addr));
        }

        if (NULL != peer_ip) {
            memset(peer_ip, 0, sizeof(*peer_ip));
        }
    }

    return ret;
}

Int32 CommSock::closeHd(Int32 fd) {
    Int32 ret = 0;
    
    if (0 <= fd) {
        ret = close(fd);
    }

    return ret;
}

Int32 CommSock::bindAddr(Int32 fd, const AddrInfo* addr) {
    Int32 ret = 0;
    
    ret = bind(fd, (const struct sockaddr*)addr->m_addr, addr->m_size);
    if (0 == ret) {
        return 0;
    } else {
        LOG_ERROR("bind_addr| ret=%d| fd=%d| addr_len=%d| error=%s|",
            ret, fd, addr->m_size, ERR_MSG());
        
        return -1; 
    }
}

Int32 CommSock::getSndBufferSize(Int32 fd, Int32* psize) {
    Int32 ret = 0;
    Int32 size = 0;
    Int32 optLen = sizeof(Int32);

    ret = getSockOpt(fd, ENUM_SOCK_LEVEL_SOL_SOCKET,
        ENUM_SOCK_OPT_SNDBUF, 
        &size, &optLen);
    
    if (0 == ret) {
        LOG_INFO("get_snd_buffer_size| fd=%d| size=%d|", fd, size);

        *psize = size;
    } else {
        LOG_ERROR("get_snd_buffer_size| fd=%d| error=%s|", fd, ERR_MSG());

        *psize = 0; 
    }

    return ret;
}

Int32 CommSock::setSndBufferSize(Int32 fd, Int32 size) {
    Int32 ret = 0;
    Int32 optLen = sizeof(Int32);

    ret = setSockOpt(fd, ENUM_SOCK_LEVEL_SOL_SOCKET,
        ENUM_SOCK_OPT_SNDBUF, 
        &size, optLen);
    
    if (0 == ret) {
        LOG_INFO("set_snd_buffer_size| fd=%d| size=%d|", fd, size);
    } else {
        LOG_ERROR("set_snd_buffer_size| fd=%d| size=%d| error=%s|", 
            fd, size, ERR_MSG());
    }

    return ret;
}

Int32 CommSock::getRcvBufferSize(Int32 fd, Int32* psize) {
    int ret = 0;
    int size = 0;
    Int32 optLen = sizeof(Int32);

    ret = getSockOpt(fd, ENUM_SOCK_LEVEL_SOL_SOCKET,
        ENUM_SOCK_OPT_RCVBUF, 
        &size, &optLen);
    
    if (0 == ret) {
        LOG_INFO("get_rcv_buffer_size| fd=%d| size=%d|", fd, size);

        *psize = size;
    } else {
        LOG_ERROR("get_rcv_buffer_size| fd=%d| error=%s|", fd, ERR_MSG());

        *psize = 0;
    }

    return ret;
}

Int32 CommSock::setRcvBufferSize(Int32 fd, Int32 size) {
    Int32 ret = 0;
    Int32 optLen = sizeof(Int32);

    ret = setSockOpt(fd, ENUM_SOCK_LEVEL_SOL_SOCKET,
        ENUM_SOCK_OPT_RCVBUF, 
        &size, optLen);
    
    if (0 == ret) {
        LOG_INFO("set_rcv_buffer_size| fd=%d| size=%d|", fd, size);
    } else {
        LOG_ERROR("set_rcv_buffer_size| fd=%d| size=%d| error=%s|", 
            fd, size, ERR_MSG());
    }

    return ret;
}

Int32 CommSock::devName2Index(const Char* name, Int32* pIndex) {
    Int32 n = 0;
    
    n = if_nametoindex(name);
    if (0 < n) {
        *pIndex = n;
        return 0;
    } else {
        *pIndex = 0;
        return -1;
    }
}

Int32 CommSock::devIndex2Name(Int32 index, 
    Char (&devname)[MAX_DEV_NAME_SIZE]) {
    const Char* psz = NULL;
    
    psz = if_indextoname(index, devname);
    if (psz == devname) {
        return 0;
    } else {
        devname[0] = '\0';
        return -1;
    }
}
