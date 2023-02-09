#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/timerfd.h>
#include<sys/eventfd.h>
#include<errno.h>
#include<poll.h>
#include<stdlib.h>
#include<time.h>
#include"sockutil.h"
#include"datatype.h"
#include"sock/tcputil.h"


Int32 creatTcpSrv(const TcpParam* param) {
    Int32 ret = 0;
    Int32 fd = -1;
    IpInfo myIp;

    myIp.m_port = param->m_port;
    strncpy(myIp.m_ip, param->m_ip, sizeof(myIp.m_ip));
    
    ret = TcpUtil::creatSvc(&myIp, &fd);
    if (0 == ret) {
        return fd;
    } else {
        return -1;
    }
}

Int32 acceptCli(Int32 listener) {
    Int32 fd = -1;
    EnumSockState stat = ENUM_SOCK_STAT_END;
    
    stat = TcpUtil::acceptTcp(listener, &fd, NULL);
    if (ENUM_SOCK_STAT_IN_PROGRESS == stat) {
        return fd;
    } else {
        return -1;
    }
}

Int32 buildParam(const Char ip[], Int32 port, TcpParam* param) {
    Int32 ret = 0;
    IpInfo ipInfo;
    AddrInfo addr;

    ipInfo.m_port = port;
    strncpy(ipInfo.m_ip, ip, sizeof(ipInfo.m_ip));
    
    ret = CommSock::ip2Addr(ENUM_DOMAIN_IP_V4, 
        ENUM_SOCK_TYPE_TCP, &ipInfo, &addr);
    if (0 == ret) {
        param->m_port = port;
        strncpy(param->m_ip, ip, sizeof(param->m_ip));
        memcpy(param->m_addr, addr.m_addr, addr.m_size);
        param->m_addr_len = addr.m_size;

        return 0;
    } else {
        return ret;
    }
}

Int32 getLocalInfo(Int32 fd, int* port, Char ip[], Int32 maxLen) {
    Int32 ret = 0;
    IpInfo ipInfo;

    ret = CommSock::getLocalInfo(fd, &ipInfo, NULL);
    if (0 == ret) {
        *port = ipInfo.m_port;
        strncpy(ip, ipInfo.m_ip, maxLen);
    }

    return ret;
}

Int32 getPeerInfo(Int32 fd, int* port, Char ip[], Int32 maxLen) {
    Int32 ret = 0;
    IpInfo ipInfo;

    ret = CommSock::getPeerInfo(fd, &ipInfo, NULL);
    if (0 == ret) {
        *port = ipInfo.m_port;

        strncpy(ip, ipInfo.m_ip, maxLen);
    }

    return ret;
}

Int32 closeHd(Int32 fd) {
    Int32 ret = 0;
    
    ret = CommSock::closeHd(fd); 
    return ret;
}

Int32 sendTcp(Int32 fd, const Void* buf, Int32 len) {
    Int32 sndlen = 0;
    
    sndlen = TcpUtil::sendTcp(fd, buf, len);
    return sndlen; 
}

Int32 recvTcp(Int32 fd, Void* buf, Int32 maxlen) {
    Int32 rdlen = 0;

    rdlen = TcpUtil::recvTcp(fd, buf, maxlen); 
    return rdlen;
}

int recvVec(int fd, struct iovec* iov, int size, int maxlen) {
    int rdlen = 0;
    struct msghdr msg;

    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = iov;
    msg.msg_iovlen = size;

    rdlen = recvmsg(fd, &msg, 0);
    if (0 < rdlen) {
        LOG_DEBUG("recvVec| fd=%d| maxlen=%d| rdlen=%d| msg=ok|",
            fd, maxlen, rdlen);
	} else if (-1 == rdlen && EAGAIN == errno) {
		rdlen = 0; 
	} else if (0 == rdlen) { 
	    LOG_ERROR("recvVec| fd=%d| maxlen=%d| error=eof|",
            fd, maxlen);
		rdlen = -2;
	} else {
		LOG_ERROR("recvVec| fd=%d| maxlen=%d| error=%s|",
            fd, maxlen, ERR_MSG());
		rdlen = -1;
	}

    return rdlen;
}

int sendVec(int fd, struct iovec* iov, int size, int maxlen) {
    int sndlen = 0;
    int flag = MSG_NOSIGNAL;
    struct msghdr msg;

    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = iov;
    msg.msg_iovlen = size;

    sndlen = sendmsg(fd, &msg, flag);
    if (0 < sndlen) {
        LOG_DEBUG("sendVec| fd=%d| size=%d| maxlen=%d| sndlen=%d| msg=ok|",
            fd, size, maxlen, sndlen);
	} else if (0 == sndlen || EAGAIN == errno) {
		sndlen = 0;
	} else {
		LOG_ERROR("sendVec| fd=%d| size=%d| maxlen=%d| error=%s|",
            fd, size, maxlen, ERR_MSG());
		sndlen = -1;
	}

    return sndlen;
}

Int32 peekTcp(Int32 fd, Void* buf, Int32 maxlen) {
    Int32 rdlen = 0;

    rdlen = recv(fd, buf, maxlen, MSG_PEEK);
    if (0 < rdlen) {
        LOG_INFO("peekTcp| fd=%d| maxlen=%d| rdlen=%d| msg=ok|",
            fd, maxlen, rdlen);
        
        return rdlen;
    } else if (0 == rdlen) {
        /* end of read */
        LOG_INFO("peekTcp| fd=%d| maxlen=%d| error=eof|",
            fd, maxlen);
        return -2;
    } else if (EAGAIN == errno || EINTR == errno) {
        return 0;
    } else {
        LOG_INFO("peekTcp| fd=%d| maxlen=%d| error=%s|",
            fd, maxlen, ERR_MSG());
        
        return -1;
    }
}


Int32 writeEvent(int fd) {
    Int32 ret = 0;
    Int32 len = sizeof(Uint64);
    Uint64 val = 1;

    ret = write(fd, &val, len);
    if (ret == len) {
        return 0;
    } else {
        return -1;
    }
}

Int32 readEvent(int fd, Uint32* pVal) {
    Int32 ret = 0;
    Int32 len = sizeof(Uint64);
    Uint64 val = 1;

    ret = read(fd, &val, len);
    if (ret == len) {
        *pVal = (Uint32)val;
        
        return 0;
    } else {
        *pVal = 0;
        
        return -1;
    }
}

Int32 creatTimerFd(Int32 ms) {
    Int32 ret = 0;
    Int32 sec = 0;
    Int32 fd = -1;
    struct itimerspec value;

    if (1000 <= ms) {
        sec = ms / 1000;
        ms = ms % 1000;
    }

    fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (0 > fd) {
        return -1;
    }
    
    value.it_value.tv_sec = 0;
	value.it_value.tv_nsec = 1;
    value.it_interval.tv_sec = sec;
	value.it_interval.tv_nsec = ms * 1000000;
    ret = timerfd_settime(fd, 0, &value, NULL);
    if (0 != ret) {
        close(fd);
        return -1;
    }

    return fd;
}

Int32 creatEventFd() {
    Int32 fd = -1; 

    fd = eventfd(1, EFD_NONBLOCK);
    if (0 > fd) {
        return -1;
    } 
    
    return fd;
}

Void waitEvent(Int32 fd, Int32 millsec) {
    Int32 ret = 0;
    Uint32 cnt = 0;
    struct pollfd fds;

    fds.fd = fd;
    fds.events = POLLIN;
    fds.revents = 0;
    
    ret = poll(&fds, 1, millsec);
    if (0 < ret && (POLLIN & fds.revents)) {
        readEvent(fd, &cnt);
    }
}

