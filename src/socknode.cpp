#include"socknode.h"
#include"objcenter.h"
#include"director.h"
#include"cache.h"
#include"sockutil.h"
#include"objcenter.h"
#include"msgtype.h"


struct SockNodePriv {
    SockNode m_pub;

    Director* m_director;
    ObjCenter* m_center;
    Int32 m_sock_fd;
};

static Int32 parse(NodeBase* node, Byte* data, Int32 len) {
    Int32 ret = 0;
    SockNodePriv* _this = (SockNodePriv*)node;

    ret = _this->m_pub.parsePkg(&_this->m_pub, data, len);
    return ret;
}

METHOD(NodeBase, getFd, Int32, SockNodePriv* _this) {
    return _this->m_sock_fd;
}

METHOD(NodeBase, readNode, EnumSockRet, SockNodePriv* _this) { 
    EnumSockRet ret = ENUM_SOCK_MARK_FINISH;

    ret = _this->m_center->readSock(&_this->m_pub.m_base, &parse);

    return ret;
}

METHOD(NodeBase, writeNode, EnumSockRet, SockNodePriv* _this) {
	EnumSockRet ret = ENUM_SOCK_MARK_FINISH;

    ret = _this->m_center->writeSock(&_this->m_pub.m_base);

    return ret;
}

METHOD(NodeBase, dealMsg, Int32, SockNodePriv* , CacheHdr* ) {
    
    return ENUM_ERR_OK;
}

METHOD(NodeBase, dealCmd, Void, SockNodePriv* , CacheHdr* ) {
    
    return;
}

METHOD(NodeBase, onClose, Void, SockNodePriv* ) {
}

METHOD(NodeBase, destroy, Void, SockNodePriv* _this) {
        
    ObjCenter::finishNode(&_this->m_pub.m_base); 

    closeHd(_this->m_sock_fd);
    I_FREE(_this);
}

METHOD(SockNode, parsePkg, Int32, SockNodePriv* _this,
    Byte* data, Int32 len) {
    CacheHdr* hdr = NULL;

    hdr = MsgCenter::creatSendMsg(data, len, NULL, 0, NULL);
    
    _this->m_director->send(&_this->m_pub.m_base, hdr);
    return ENUM_ERR_OK;
}


METHOD(SockNode, onAccept, Int32, SockNodePriv* ) {
    return 0;
}

METHOD(SockNode, onConnOk, Int32, SockNodePriv* ) {
    return 0;
}

METHOD(SockNode, onConnFail, Void, SockNodePriv* ) {
    return;
}

SockNode* creatSockNode(Int32 fd, Director* director) {
    SockNodePriv* _this = NULL;

    I_NEW(SockNodePriv, _this); 

    ObjCenter::initNode(&_this->m_pub.m_base);

    _this->m_pub.m_base.m_node_type = ENUM_NODE_SOCKET;
    _this->m_sock_fd = fd;
    _this->m_director = director;
    _this->m_center = director->getCenter();
    
    director->condition(&_this->m_pub.m_base.m_rcv_task, BIT_EVENT_READ);
    director->condition(&_this->m_pub.m_base.m_snd_task, BIT_EVENT_WRITE);
    
    _this->m_pub.m_base.getFd = _getFd;
    _this->m_pub.m_base.readNode = _readNode;
    _this->m_pub.m_base.writeNode = _writeNode;

    _this->m_pub.m_base.dealMsg = _dealMsg;
    _this->m_pub.m_base.dealCmd = _dealCmd;
    
    _this->m_pub.m_base.onClose = _onClose;
    _this->m_pub.m_base.destroy = _destroy;

    _this->m_pub.parsePkg = _parsePkg;
    _this->m_pub.onAccept = _onAccept;
    _this->m_pub.onConnOk = _onConnOk;
    _this->m_pub.onConnFail = _onConnFail;

    return &_this->m_pub; 
}

