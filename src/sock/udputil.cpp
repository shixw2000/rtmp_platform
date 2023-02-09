#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<netinet/in.h>
#include<netinet/ip.h>
#include<unistd.h>
#include<fcntl.h>
#include"udputil.h"
#include"commsock.h"
#include"misc.h"


Int32 UdpUtil::creatUdp(Int32* pFd) {
    Int32 ret = 0;
    Int32 fd = -1;

    do { 
        ret = CommSock::creatSock(ENUM_DOMAIN_IP_V4, 
            ENUM_SOCK_TYPE_UDP, 0, &fd);
        if (0 != ret) {
            break;
        }

        CommSock::setNonBlock(fd);
        //CommSock::setReuse(fd);

        *pFd = fd;
        return 0;
    } while (0);

    CommSock::closeHd(fd);
    return ret;
}

Int32 UdpUtil::creatUniSvc(Int32 port, const Char* uniIP, Int32* pfd) {
    Int32 ret = 0;
    Int32 fd = -1;
    IpInfo ipInfo;
    AddrInfo addr;

    do {
        ipInfo.m_port = port;
        strncpy(ipInfo.m_ip, uniIP, sizeof(ipInfo.m_ip));
        
        ret = CommSock::ip2Addr(ENUM_DOMAIN_IP_V4, 
            ENUM_SOCK_TYPE_UDP, &ipInfo, &addr);
        if (0 != ret) {
            break;
        } 
        
        ret = UdpUtil::creatUdp(&fd);
        if (0 != ret) {
            break;
        } 

        ret = CommSock::bindAddr(fd, &addr);
        if (0 != ret) {
            break;
        }

        LOG_INFO("creat_uni_svc| fd=%d| port=%d| ip=%s| msg=ok|",
            fd, port, uniIP);

        *pfd = fd;
        return 0;
    } while (0);

    LOG_ERROR("creat_uni_svc| ret=%d| port=%d| ip=%s| msg=fail|", 
        ret, port, uniIP); 

    CommSock::closeHd(fd);
    *pfd = -1;
    return ret;
}

Int32 UdpUtil::creatMultiSvc(Int32 port, const Char* multiIP,
    const Char* localIP, Int32* pfd) {
    Int32 ret = 0;
    Int32 fd = -1;
    IpInfo ipInfo;
    AddrInfo addr;

    do {
        ipInfo.m_port = port;
        strncpy(ipInfo.m_ip, multiIP, sizeof(ipInfo.m_ip));
        
        ret = CommSock::ip2Addr(ENUM_DOMAIN_IP_V4, 
            ENUM_SOCK_TYPE_UDP, &ipInfo, &addr);
        if (0 != ret) {
            break;
        } 
        
        ret = UdpUtil::creatUdp(&fd);
        if (0 != ret) {
            break;
        } 

        ret = CommSock::bindAddr(fd, &addr);
        if (0 != ret) {
            break;
        }

        ret = UdpUtil::operMemship(fd, localIP, multiIP, TRUE);
        if (0 != ret) {
            break;
        }

        LOG_INFO("creat_multi_svc| fd=%d| port=%d| multi_ip=%s|"
            " local_ip=%s| msg=ok|",
            fd, port, multiIP, localIP);

        *pfd = fd;
        return 0;
    } while (0);

    LOG_INFO("creat_multi_svc| ret=%d| port=%d| multi_ip=%s|"
            " local_ip=%s| msg=fail|",
            ret, port, multiIP, localIP);

    CommSock::closeHd(fd);
    *pfd = -1;
    return ret;
}


Int32 UdpUtil::sendTo(Int32 fd, const Void* buf, 
    Int32 maxlen, const AddrInfo* peer) {
    Int32 sndlen = 0;
    Int32 flags = MSG_NOSIGNAL;
    IpInfo ip; 

    CommSock::addr2IP(peer, &ip);
    
    if (0 < maxlen) {
        sndlen = sendto(fd, buf, maxlen, flags, 
            (struct sockaddr*)peer->m_addr,
            peer->m_size);
        if (0 <= sndlen) { 
            LOG_INFO("sendTo| fd=%d| max_len=%d| snd_len=%d|"
                " peer=%s:%d| msg=ok|",
                fd, maxlen, sndlen,
                ip.m_ip, ip.m_port);
        } else if (EAGAIN == errno) {
            /* blocked and wait  */
            sndlen = 0;
        } else if (ENOBUFS == errno || EMSGSIZE == errno) {
            LOG_WARN("sendTo| fd=%d| maxlen=%d| snd_len=%d|"
                " peer=%s:%d| msg=invalid:%s|",
                fd, maxlen, sndlen, 
                ip.m_ip, ip.m_port,
                ERR_MSG());
            
            /* invalid packet parameter */
            sndlen = -2;
        } else {
            LOG_ERROR("sendTo| fd=%d| maxlen=%d| snd_len=%d|"
                " peer=%s:%d| error=%s|",
                fd, maxlen, sndlen,
                ip.m_ip, ip.m_port,
                ERR_MSG());
            
            sndlen = -1;
        }
    } 

    return sndlen;
}

Int32 UdpUtil::recvFrom(Int32 fd, Void* buf, 
    Int32 maxlen, AddrInfo* peer) {
    Int32 rdlen = 0;
    Int32 flags = MSG_TRUNC;    // return the actual packet length
    socklen_t addrLen = 0;
    IpInfo ip; 
    AddrInfo addr;

    if (0 < maxlen) { 
        addrLen = sizeof(addr.m_addr);
        rdlen = recvfrom(fd, buf, maxlen, flags,
            (struct sockaddr*)addr.m_addr, 
            &addrLen);
        if (0 <= rdlen) {
            addr.m_size = addrLen; 
            CommSock::addr2IP(&addr, &ip);
            
            if (rdlen <= maxlen) {
                /* valid recv */ 
                LOG_DEBUG("recvFrom| fd=%d| maxlen=%d| rdlen=%d|"
                " peer=%s:%d| msg=ok|",
                    fd, maxlen, rdlen,
                    ip.m_ip, ip.m_port);
                
                if (NULL != peer) {
                    CommSock::copyAddr(&addr, peer);
                }
            } else {
                LOG_DEBUG("recvFrom| fd=%d| maxlen=%d| expect_len=%d|"
                    " peer=%s:%d| msg=data truncated error|",
                    fd, maxlen, rdlen,
                    ip.m_ip, ip.m_port);

                rdlen = -1;
            } 
        } else if (EAGAIN == errno) {
            /* empty and must wait to  */
            rdlen = 0;
        } else {
            LOG_ERROR("recvFrom| fd=%d| maxlen=%d| error=%s|",
                fd, maxlen, ERR_MSG());
            
            rdlen = -1;
        }
    } 

    return rdlen;
}

Int32 UdpUtil::recvUdp(int fd, AddrInfo* peer, 
    struct iovec* iov, int iov_size) {
    Int32 maxlen = 0;
    Int32 rdlen = 0;
    Int32 flags = 0;
    struct msghdr msg;
    AddrInfo addr;
    IpInfo ip;

    /* calc the total length of buffer */
    for (int i=0; i<iov_size; ++i) {
        maxlen += iov[i].iov_len;
    }

    if (0 >= maxlen) {
        return 0;
    }

    memset(&msg, 0, sizeof(msg));

    msg.msg_name = addr.m_addr;
    msg.msg_namelen = sizeof(addr.m_addr);
    
    msg.msg_iov = iov;
    msg.msg_iovlen = iov_size;

    /* to check if truncated */
    flags = MSG_TRUNC;
    rdlen = recvmsg(fd, &msg, flags);
    if (0 < rdlen) {
        addr.m_size = msg.msg_namelen;
        CommSock::addr2IP(&addr, &ip);
        
        if (rdlen <= maxlen && !(MSG_TRUNC & msg.msg_flags)) {
            /* valid recv */ 
            if (NULL != peer) {
                CommSock::copyAddr(&addr, peer);
            }

            LOG_INFO("recv_udp| fd=%d| iov_size=%d|"
                " maxlen=%d| rdlen=%d|"
                " rflag=0x%x| peer=%s:%d| msg=ok|",
                fd, iov_size, 
                maxlen, rdlen, 
                msg.msg_flags,
                ip.m_ip, ip.m_port);
        } else {
            LOG_ERROR("recv_udp| fd=%d| iov_size=%d|"
                " maxlen=%d| expect_len=%d|"
                " rflag=0x%x| peer=%s:%d| msg=data truncated error|",
                fd, iov_size,
                maxlen, rdlen, 
                msg.msg_flags,
                ip.m_ip, ip.m_port);

            rdlen = -1;
        } 
	} else if (0 == rdlen || EAGAIN == errno) {
		rdlen = 0; 
	} else {
		LOG_ERROR("recv_udp| fd=%d| iov_size=%d| maxlen=%d|"
            " rflag=0x%x| error=%s|",
            fd, iov_size, maxlen,
            msg.msg_flags, ERR_MSG());
		rdlen = -1;
	}

    return rdlen;
}

Int32 UdpUtil::sendUdp(int fd, const AddrInfo* peer,
    struct iovec* iov, int iov_size) {
    Int32 maxlen = 0;
    Int32 sndlen = 0;
    Int32 flags = MSG_NOSIGNAL;
    struct msghdr msg;
    IpInfo ip; 

    CommSock::addr2IP(peer, &ip);

    /* calc the total length of buffer */
    for (int i=0; i<iov_size; ++i) {
        maxlen += iov[i].iov_len;
    }

    if (0 >= maxlen) {
        return 0;
    }

    memset(&msg, 0, sizeof(msg));
    
    if (NULL != peer) {
        msg.msg_name = (Void*)peer->m_addr;
        msg.msg_namelen = peer->m_size;
    }
    
    msg.msg_iov = iov;
    msg.msg_iovlen = iov_size;

    sndlen = sendmsg(fd, &msg, flags);
    if (0 < sndlen) {
        LOG_INFO("send_udp| fd=%d| iov_size=%d| maxlen=%d| sndlen=%d|"
            " peer=%s:%d| msg=ok|",
            fd, iov_size, maxlen, sndlen,
            ip.m_ip, ip.m_port);
	} else if (0 == sndlen || EAGAIN == errno) {
		sndlen = 0;
	} else {
		LOG_ERROR("send_udp| fd=%d| iov_size=%d| maxlen=%d|"
            " peer=%s:%d| error=%s|",
            fd, iov_size, maxlen, 
            ip.m_ip, ip.m_port,
            ERR_MSG());
		sndlen = -1;
	}

    return sndlen;
}

/* see also getifaddrs */
Int32 UdpUtil::operMemship(Int32 fd, const Char* localIp,
    const Char* groupIp, Bool added) {
    Int32 ret = 0;
    Int32 optLen = 0; 
    EnumSockOptName optname = ENUM_SOCK_OPT_NAME_END;
    struct ip_mreqn req;
    Byte addr[MAX_ADDR_SIZE] = {0};
    
    memset(&req, 0, sizeof(req));

    do {
        ret = CommSock::ipstr2N(ENUM_DOMAIN_IP_V4, localIp, addr);
        if (0 == ret) {
            memcpy(&req.imr_address, addr, sizeof(req.imr_address));
        } else {
            break;
        }

        ret = CommSock::ipstr2N(ENUM_DOMAIN_IP_V4, groupIp, addr);
        if (0 == ret) {
            memcpy(&req.imr_multiaddr, addr, sizeof(req.imr_multiaddr));
        } else {
            break;
        } 

        if (added) {
            optname = ENUM_IP_OPT_ADD_MEM;
        } else {
            optname = ENUM_IP_OPT_DROP_MEM;
        }

        optLen = (Int32)sizeof(req);
        ret = CommSock::setSockOpt(fd, ENUM_SOCK_LEVEL_IPPROTO_IP,
            optname, &req, optLen);
        if (0 == ret) {
            LOG_INFO("oper_member| is_add=%d| fd=%d| unicast_ip=%s|"
                " multi_ip=%s| msg=ok|",
                added, fd, localIp, groupIp);
                
            return 0;
        } else {
            break;
        }
    } while (0); 
    
    LOG_ERROR("oper_member| is_add=%d| ret=%d| fd=%d| unicast_ip=%s|"
        " multicast_ip=%s| msg=add failed|",
        added, ret, fd, localIp, groupIp);
    
    return ret;
}

