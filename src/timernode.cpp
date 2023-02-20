#include"timernode.h"
#include"objcenter.h"
#include"director.h"
#include"ticktimer.h"
#include"msgtype.h"
#include"sockutil.h"
#include"cache.h"


template<EnumCtrlType type>
class CtrlTimer : public TickTimer {
public:
    explicit CtrlTimer(Director* director) : m_director(director) {}
        
protected:
    virtual Void doTimeout(struct TimerEle* ele) {
        m_director->procTimeout<type>(ele);
    }

private:
    Director* m_director;
};

struct TimerNodePriv {
    TimerNode m_pub;

    Director* m_director;
    TickTimer* m_timer[CTRL_TYPE_END];
    Int32 m_timer_fd;
};


METHOD(NodeBase, getFd, Int32, TimerNodePriv* _this) {
    return _this->m_timer_fd;
}

METHOD(NodeBase, readNode, EnumSockRet, TimerNodePriv* _this) {
    Int32 ret = 0;
	Uint32 val = 0; 

    ret = readEvent(_this->m_timer_fd, &val);
    if (0 == ret) {
        _this->m_director->readTimer(val);
    }

    return ENUM_SOCK_MARK_FINISH;
}

METHOD(NodeBase, writeNode, EnumSockRet, TimerNodePriv* _this) {
    list_head* list = NULL;
    list_node* pos = NULL;
    list_node* n = NULL;
    CacheHdr* hdr = NULL;

    list = &_this->m_pub.m_base.m_snd_que;
    if (!list_empty(list)) {
        list_for_each_safe(pos, n, list) {
            list_del(pos, list);

            hdr = CacheCenter::cast(pos);
            _this->m_director->procTimer(CTRL_TYPE_SND, hdr);
        }
    }

    return ENUM_SOCK_MARK_FINISH;
}

METHOD(NodeBase, onClose, Void, TimerNodePriv* _this) {
    _this->m_director->notifyExit(&_this->m_pub.m_base);
}

METHOD(NodeBase, destroy, Void, TimerNodePriv* _this) {
    for (int i=0; i<CTRL_TYPE_END; ++i) {
        if (NULL != _this->m_timer[i]) {
            I_FREE(_this->m_timer[i]);
        }
    }

    if (0 <= _this->m_timer_fd) {
        closeHd(_this->m_timer_fd);
        _this->m_timer_fd= -1;
    }
    
    ObjCenter::finishNode(&_this->m_pub.m_base); 
    I_FREE(_this);
}

METHOD(TimerNode, addTimer, Void, TimerNodePriv* _this,
    EnumCtrlType type, TimerEle* ele) {
    _this->m_timer[type]->addTimer(ele);
}

METHOD(TimerNode, stopTimer, Void, TimerNodePriv* _this,
    EnumCtrlType type) {
    _this->m_timer[type]->stop();
}

METHOD(TimerNode, doTick, Void, TimerNodePriv* _this,
    EnumCtrlType type, Uint32 cnt) {
    _this->m_timer[type]->tick(cnt);
}

TimerNode* creatTimerNode(Director* director) {
    Int32 fd = -1;
    TimerNodePriv* _this = NULL;

    fd = creatTimerFd(DEF_TIMER_TICK_MS);
    if (0 > fd) {
        return NULL;
    }

    I_NEW(TimerNodePriv, _this);
    
    memset(_this, 0, sizeof(*_this));
    
    ObjCenter::initNode(&_this->m_pub.m_base, ENUM_NODE_TIMER);

    _this->m_director = director;
    _this->m_timer_fd = fd;

    director->condition(&_this->m_pub.m_base.m_rcv_task, BIT_EVENT_READ);
    _this->m_pub.m_base.m_can_wr = TRUE;

    I_NEW_P(CtrlTimer<CTRL_TYPE_RCV>, _this->m_timer[CTRL_TYPE_RCV], director);
    I_NEW_P(CtrlTimer<CTRL_TYPE_SND>, _this->m_timer[CTRL_TYPE_SND], director);
    I_NEW_P(CtrlTimer<CTRL_TYPE_DEAL>, _this->m_timer[CTRL_TYPE_DEAL], director);
    I_NEW_P(CtrlTimer<CTRL_TYPE_CMD>, _this->m_timer[CTRL_TYPE_CMD], director);
    
    _this->m_pub.m_base.getFd = _getFd;
    _this->m_pub.m_base.readNode = _readNode;
    _this->m_pub.m_base.writeNode = _writeNode;

    _this->m_pub.m_base.onClose = _onClose;
    _this->m_pub.m_base.destroy = _destroy;

    _this->m_pub.addTimer = _addTimer;
    _this->m_pub.stopTimer = _stopTimer;
    _this->m_pub.doTick = _doTick;

    return &_this->m_pub;
}


