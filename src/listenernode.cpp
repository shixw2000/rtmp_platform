#include"listenernode.h"
#include"objcenter.h"
#include"director.h"
#include"cache.h"
#include"sockutil.h"
#include"socknode.h"
#include"rtmpnode.h"


struct ListenerNodePriv {
    ListenerNode m_pub;

    Director* m_director;
    Int32 m_listener_fd;
};

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

METHOD(NodeBase, getFd, Int32, ListenerNodePriv* _this) {
    return _this->m_listener_fd;
}

METHOD(NodeBase, readNode, EnumSockRet, ListenerNodePriv* _this) {
	//acceptSock(_this->m_listener_fd, _this->m_director);
    acceptRtmp(_this->m_listener_fd, _this->m_director);

    return ENUM_SOCK_MARK_FINISH;
}

METHOD(NodeBase, dealMsg, Int32, ListenerNodePriv* , CacheHdr* ) {
    
    return ENUM_ERR_OK;
}

METHOD(NodeBase, dealCmd, Void, ListenerNodePriv* , CacheHdr* ) {
    
    return;
}

METHOD(NodeBase, onClose, Void, ListenerNodePriv* ) {
}

METHOD(NodeBase, destroy, Void, ListenerNodePriv* _this) {
    
    ObjCenter::finishNode(&_this->m_pub.m_base); 

    closeHd(_this->m_listener_fd);
    I_FREE(_this);
}

ListenerNode* creatListenerNode(Int32 fd, Director* director) {
    ListenerNodePriv* _this = NULL;

    I_NEW(ListenerNodePriv, _this);

    ObjCenter::initNode(&_this->m_pub.m_base);

    _this->m_director = director;
    _this->m_listener_fd = fd;

    director->condition(&_this->m_pub.m_base.m_rcv_task, BIT_EVENT_READ);
    
    _this->m_pub.m_base.getFd = _getFd;
    _this->m_pub.m_base.readNode = _readNode;

    _this->m_pub.m_base.dealMsg = _dealMsg;
    _this->m_pub.m_base.dealCmd = _dealCmd;
    
    _this->m_pub.m_base.onClose = _onClose;
    _this->m_pub.m_base.destroy = _destroy;

    return &_this->m_pub;
}


