#include"listenernode.h"
#include"objcenter.h"
#include"director.h"
#include"cache.h"
#include"sockutil.h"
#include"socknode.h"
#include"rtmpnode.h"


static Void acceptSock(Int32 listener_fd, Director* director) { 
    Int32 ret = 0;
    Int32 newfd = -1;
    int peer_port = 0;
    Char peer_ip[DEF_IP_SIZE];
    SockNode* node = NULL;

    newfd = acceptCli(listener_fd);
    while (0 <= newfd) { 
        getPeerInfo(newfd, peer_ip, &peer_port);
        
        node = creatSockNode(newfd, director);
        if (NULL != node) {
            ret = node->onAccept(node);
            if (0 == ret) {
                LOG_DEBUG("socket_accept| fd=%d| addr=%s:%d| msg=ok|",
                    newfd, peer_ip, peer_port);
                
                director->registTask(&node->m_base, EVENT_TYPE_ALL);
            } else {
                LOG_ERROR("socket_accept| fd=%d| addr=%s:%d|"
                    " msg=onAccept error|",
                    newfd, peer_ip, peer_port);
                
                node->m_base.destroy(&node->m_base);
            }
        } else {
            LOG_ERROR("socket_accept| fd=%d| addr=%s:%d|"
                " msg=creat node error|",
                newfd, peer_ip, peer_port);
            
            closeHd(newfd);
        }

        newfd = acceptCli(listener_fd);
    }
}

static Void acceptRtmp(Int32 listener_fd, Director* director) { 
    Int32 ret = 0;
    Int32 newfd = -1;
    int peer_port = 0;
    Char peer_ip[DEF_IP_SIZE];
    RtmpNode* node = NULL;

    newfd = acceptCli(listener_fd);
    while (0 <= newfd) { 
        getPeerInfo(newfd, peer_ip, &peer_port);
        
        node = creatRtmpNode(newfd, director);
        if (NULL != node) {
            ret = node->onAccept(node);
            if (0 == ret) {
                LOG_DEBUG("rtmp_accept| fd=%d| addr=%s:%d| msg=ok|",
                    newfd, peer_ip, peer_port);
                
                director->registTask(&node->m_base, EVENT_TYPE_ALL);
            } else {
                LOG_ERROR("rtmp_accept| fd=%d| addr=%s:%d|"
                    " msg=onAccept error|",
                    newfd, peer_ip, peer_port);
                
                node->m_base.destroy(&node->m_base);
            }
        } else {
            LOG_ERROR("rtmp_accept| fd=%d| addr=%s:%d|"
                " msg=creat node error|",
                newfd, peer_ip, peer_port);
            
            closeHd(newfd);
        }

        newfd = acceptCli(listener_fd);
    }
}


struct RtmpListenerPriv {
    ListenerNode m_pub;

    Director* m_director;
    Int32 m_listener_fd;
};

METHOD(NodeBase, rtmp_getFd, Int32, RtmpListenerPriv* _this) {
    return _this->m_listener_fd;
}

METHOD(NodeBase, rtmp_readNode, EnumSockRet, RtmpListenerPriv* _this) {
    acceptRtmp(_this->m_listener_fd, _this->m_director);

    return ENUM_SOCK_MARK_FINISH;
}


METHOD(NodeBase, rtmp_destroy, Void, RtmpListenerPriv* _this) {
    
    ObjCenter::finishNode(&_this->m_pub.m_base); 

    closeHd(_this->m_listener_fd);
    I_FREE(_this);
}

ListenerNode* creatRtmpListener(Int32 fd, Director* director) {
    RtmpListenerPriv* _this = NULL;

    I_NEW(RtmpListenerPriv, _this);

    ObjCenter::initNode(&_this->m_pub.m_base);

    _this->m_director = director;
    _this->m_listener_fd = fd;

    director->condition(&_this->m_pub.m_base.m_rcv_task, BIT_EVENT_READ);
    
    _this->m_pub.m_base.getFd = _rtmp_getFd;
    _this->m_pub.m_base.readNode = _rtmp_readNode;
    _this->m_pub.m_base.destroy = _rtmp_destroy;

    return &_this->m_pub;
}


struct SockListenerPriv {
    ListenerNode m_pub;

    Director* m_director;
    Int32 m_listener_fd;
};


METHOD(NodeBase, sock_getFd, Int32, SockListenerPriv* _this) {
    return _this->m_listener_fd;
}

METHOD(NodeBase, sock_readNode, EnumSockRet, SockListenerPriv* _this) {
	acceptSock(_this->m_listener_fd, _this->m_director);

    return ENUM_SOCK_MARK_FINISH;
}


METHOD(NodeBase, sock_destroy, Void, SockListenerPriv* _this) {
    
    ObjCenter::finishNode(&_this->m_pub.m_base); 

    closeHd(_this->m_listener_fd);
    I_FREE(_this);
}

ListenerNode* creatSockListener(Int32 fd, Director* director) {
    SockListenerPriv* _this = NULL;

    I_NEW(SockListenerPriv, _this);

    ObjCenter::initNode(&_this->m_pub.m_base);

    _this->m_director = director;
    _this->m_listener_fd = fd;

    director->condition(&_this->m_pub.m_base.m_rcv_task, BIT_EVENT_READ);
    
    _this->m_pub.m_base.getFd = _sock_getFd;
    _this->m_pub.m_base.readNode = _sock_readNode;
    _this->m_pub.m_base.destroy = _sock_destroy;

    return &_this->m_pub;
}


