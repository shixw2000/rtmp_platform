#include"ctrlnode.h"
#include"objcenter.h"
#include"director.h"
#include"cache.h"


struct CtrlNodePriv {
    CtrlNode m_pub;

    list_head m_rcv_que_tmp;
    list_head m_rcv_que; 
    Director* m_director;
};

METHOD(NodeBase, getFd, Int32, CtrlNodePriv* ) {
    return 0;
}

METHOD(NodeBase, readNode, EnumSockRet, CtrlNodePriv* _this) {
	Bool bOk = TRUE;
    list_head* list = NULL;
    list_node* pos = NULL;
    list_node* n = NULL;
    CacheHdr* hdr = NULL;

    bOk = _this->m_director->lock(&_this->m_pub.m_base);
    if (bOk) {
        if (!list_empty(&_this->m_rcv_que_tmp)) {
            list_splice_back(&_this->m_rcv_que_tmp, &_this->m_rcv_que);
        }
        
        _this->m_director->unlock(&_this->m_pub.m_base);
    }

    list = &_this->m_rcv_que;
    if (!list_empty(list)) {
        list_for_each_safe(pos, n, list) {
            list_del(pos, list);

            hdr = CacheCenter::cast(pos);
            _this->m_director->dealCtrlRcv(hdr);
        }
    }

    return ENUM_SOCK_MARK_FINISH;
}

METHOD(NodeBase, writeNode, EnumSockRet, CtrlNodePriv* _this) {
    list_head* list = NULL;
    list_node* pos = NULL;
    list_node* n = NULL;
    CacheHdr* hdr = NULL;

    list = &_this->m_pub.m_base.m_snd_que;
    if (!list_empty(list)) {
        list_for_each_safe(pos, n, list) {
            list_del(pos, list);

            hdr = CacheCenter::cast(pos);
            _this->m_director->dealCtrlSnd(hdr);
        }
    }

    return ENUM_SOCK_MARK_FINISH;
}

METHOD(NodeBase, onClose, Void, CtrlNodePriv* ) {
}

METHOD(NodeBase, destroy, Void, CtrlNodePriv* _this) {
    ObjCenter::freeMsgQue(&_this->m_rcv_que_tmp);
    ObjCenter::freeMsgQue(&_this->m_rcv_que);
    
    ObjCenter::finishNode(&_this->m_pub.m_base); 
    I_FREE(_this);
}

METHOD(CtrlNode, recv, Int32, CtrlNodePriv* _this, CacheHdr* hdr) {
    CacheCenter::queue(hdr, &_this->m_rcv_que_tmp);
    
    return 0;
}

CtrlNode* creatCtrlNode(Director* director) {
    CtrlNodePriv* _this = NULL;

    I_NEW(CtrlNodePriv, _this);

    ObjCenter::initNode(&_this->m_pub.m_base);

    INIT_LIST_HEAD(&_this->m_rcv_que_tmp);
    INIT_LIST_HEAD(&_this->m_rcv_que);
    _this->m_director = director;
    
    _this->m_pub.m_base.m_can_wr = TRUE;
    
    _this->m_pub.m_base.getFd = _getFd;
    _this->m_pub.m_base.readNode = _readNode;
    _this->m_pub.m_base.writeNode = _writeNode;
    
    _this->m_pub.m_base.onClose = _onClose;
    _this->m_pub.m_base.destroy = _destroy;

    _this->m_pub.recv = _recv;

    return &_this->m_pub;
}

