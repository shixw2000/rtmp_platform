#include"director.h"
#include"lock.h"
#include"datatype.h"
#include"receiver.h"
#include"sender.h"
#include"dealer.h"
#include"cache.h"
#include"msgtype.h"
#include"ticktimer.h"
#include"timernode.h"
#include"ctrlnode.h"
#include"sockutil.h"
#include"listenernode.h"
#include"objcenter.h"


static const Char* DEF_CTRL_THR_NAME[CTRL_TYPE_END] = {
    "receiver",
    "sender",
    "dealer",
    "director"
};

Director::Director(int capacity) : m_capacity(capacity) {
    m_lock = NULL;
    m_center = NULL;
    m_dealer = NULL;
    m_sender = NULL;
    m_recver = NULL;

    m_ctrl_node = NULL;
    m_timer_node = NULL;
    m_is_ready = FALSE;

    INIT_LIST_HEAD(&m_cmd_list);

    for (int i=0; i<CTRL_TYPE_END; ++i) {
        INIT_TIMER_ELE(&m_timer_ele[i]);
    }
}

Director::~Director() {
}

Int32 Director::init() {
    Int32 ret = 0;

    ret = TaskWorker::init();
    if (0 != ret) {
        return ret;
    }

    setName(getThrName(CTRL_TYPE_CMD));

    m_ctrl_node = creatCtrlNode(this);
    m_timer_node = creatTimerNode(this);

    I_NEW_P(GrpSpinLock, m_lock, DEF_LOCK_ORDER);
    ret = m_lock->init();
    if (0 != ret) {
        return ret;
    }

    I_NEW(ObjCenter, m_center);
    ret = m_center->init();
    if (0 != ret) {
        return ret;
    }

    I_NEW_P(Dealer, m_dealer, this);
    ret = m_dealer->init();
    if (0 != ret) {
        return ret;
    }

    I_NEW_P(Receiver, m_recver, m_capacity, this);
    ret = m_recver->init();
    if (0 != ret) {
        return ret;
    }

    I_NEW_P(Sender, m_sender, m_capacity, this);
    ret = m_sender->init();
    if (0 != ret) {
        return ret;
    }

    m_recver->addNode(&m_timer_node->m_base);

    for (int i=0; i<CTRL_TYPE_END; ++i) {
        addTimer((EnumCtrlType)i, &m_timer_ele[i],
            ENUM_TIMER_CMD_SECOND,
            DEF_SECOND_TICK_CNT);
    } 

    m_is_ready = TRUE;

    return ret;
}

Void Director::finish() {
    m_is_ready = FALSE; 
    
    if (NULL != m_dealer) {
        m_dealer->finish();
        I_FREE(m_dealer);
    }

    if (NULL != m_recver) {
        m_recver->finish();
        I_FREE(m_recver);
    }

    if (NULL != m_sender) {
        m_sender->finish();
        I_FREE(m_sender);
    }

    if (NULL != m_center) {
        m_center->finish();
        I_FREE(m_center);
    }
    
    if (NULL != m_lock) {
        m_lock->finish();
        I_FREE(m_lock);
    } 

    if (NULL != m_timer_node) {
        m_timer_node->m_base.destroy(&m_timer_node->m_base);

        m_timer_node = NULL;
    }

    if (NULL != m_ctrl_node) {
        m_ctrl_node->m_base.destroy(&m_ctrl_node->m_base);

        m_ctrl_node = NULL;
    } 

    freeNodes();
    
    TaskWorker::finish();
} 

const Char* Director::getThrName(EnumCtrlType type) const {
    return DEF_CTRL_THR_NAME[type];
}

int Director::start() {
    int ret = 0; 

    ret = m_dealer->start();
    if (0 != ret) {
        return ret;
    }

    ret = m_sender->start();
    if (0 != ret) {
        return ret;
    } 

    ret = m_recver->start();
    if (0 != ret) {
        return ret;
    } 

    ret = TaskWorker::start();
    if (0 != ret) {
        return ret;
    }

    return 0;
}

void Director::join() {
    TaskWorker::join(); 
    m_dealer->join();
    m_sender->join();
    m_recver->join();
}

void Director::stop() {
    if (m_is_ready) {
        TaskWorker::stop();
        m_recver->stop();
        m_dealer->stop();
        m_sender->stop();
    }
}

int Director::setup() { 
    return 0;
}

void Director::teardown() {
    stopTimer(CTRL_TYPE_CMD);
    
    /* if exit, then stop all threads */
    stop();
}

unsigned int Director::procTask(struct Task* task) {
    NodeBase* node = NULL;

    node = list_entry(task, NodeBase, m_cmd_task);

    procNodeQue(node);
    
    return BIT_EVENT_NORM; 
}

void Director::procTaskEnd(struct Task* task) {
    NodeBase* node = NULL;

    node = list_entry(task, NodeBase, m_cmd_task);

    LOG_INFO("proc_task_end| fd=%d| type=%d| msg=finished|",
        node->getFd(node), node->m_node_type);

    delQue(node);
    
    node->m_fd_status = ENUM_FD_CLOSED; 

    /* destroy the node */
    node->destroy(node);
    return;
}

Void Director::procNodeQue(NodeBase* node) {
    list_head* list = NULL;
    list_node* pos = NULL;
    list_node* n = NULL;
    CacheHdr* hdr = NULL;
    Bool bOk = TRUE;

    bOk = lock(node);
    if (bOk) {
        if (!list_empty(&node->m_cmd_que_tmp)) {
            list_splice_back(&node->m_cmd_que_tmp, &node->m_cmd_que);
        }
        
        unlock(node);
    }
    
    list = &node->m_cmd_que;
    if (!list_empty(list)) {
        list_for_each_safe(pos, n, list) {
            list_del(pos, list);

            hdr = CacheCenter::cast(pos);
            dealCmd(node, hdr);
        }
    }
}

Bool Director::lock(NodeBase* node) {
    int idx = node->getFd(node);

    return m_lock->lock(idx);
}

Bool Director::unlock(NodeBase* node) {
    int idx = node->getFd(node);

    return m_lock->unlock(idx);
}

CacheHdr* Director::genExchMsg(Int32 cmd, Int64 val) {
    CacheHdr* hdr = NULL;

    hdr = MsgCenter::creatExchMsg(cmd, val);

    return hdr;
}

Int64 Director::getExchVal(CacheHdr* hdr) {
    Int64 val = 0;
    
    val = MsgCenter::getExchMsgVal(hdr);
    return val;
}

Int32 Director::dispatch(NodeBase* node, CacheHdr* hdr) {
    Bool bOk = TRUE;

    bOk = lock(node);
    if (bOk) {
        CacheCenter::queue(hdr, &node->m_deal_que_tmp);
        
        unlock(node);

        m_dealer->addTask(&node->m_deal_task, BIT_EVENT_NORM);

        return 0;
    } else {
        CacheCenter::free(hdr);

        return -1;
    } 
}

Int32 Director::recv(CacheHdr* hdr) {
    Int32 ret = 0;
    Bool bOk = TRUE;

    bOk = lock(&m_ctrl_node->m_base);
    if (bOk) {
        ret = m_ctrl_node->recv(m_ctrl_node, hdr);
        
        unlock(&m_ctrl_node->m_base);

        if (0 == ret) {
            m_recver->addTask(&m_ctrl_node->m_base.m_rcv_task, BIT_EVENT_NORM);
            return 0;
        } else {
            return ret;
        }
    } else {
        CacheCenter::free(hdr);

        return -1;
    } 
}

Int32 Director::recvExch(Int32 cmd, Int64 val) {
    Int32 ret = 0;
    CacheHdr* hdr = NULL;
    
    hdr = genExchMsg(cmd, val); 
    ret = recv(hdr);
    
    return ret;
}

Int32 Director::sendExch(NodeBase* node, Int32 cmd, Int64 val) {
    Int32 ret = 0;
    CacheHdr* hdr = NULL;
    
    hdr = genExchMsg(cmd, val); 
    ret = send(node, hdr);
    
    return ret;
}

Int32 Director::readTimer(Uint32 val) {
    Int32 ret = 0;

    sendExch(&m_timer_node->m_base, ENUM_MSG_TIMER_TICK, val);
    dispatchExch(&m_timer_node->m_base, ENUM_MSG_TIMER_TICK, val); 
    directExch(&m_timer_node->m_base, ENUM_MSG_TIMER_TICK, val); 

    m_timer_node->doTick(m_timer_node, CTRL_TYPE_RCV, val);
    
    return ret;
}

Int32 Director::dispatchExch(NodeBase* node, Int32 cmd, Int64 val) {
    Int32 ret = 0;
    CacheHdr* hdr = NULL;
    
    hdr = genExchMsg(cmd, val);
    ret = dispatch(node, hdr);
    
    return ret;
}

Int32 Director::directExch(NodeBase* node, Int32 cmd, Int64 val) {
    Int32 ret = 0;
    CacheHdr* hdr = NULL;
    
    hdr = genExchMsg(cmd, val);
    ret = direct(node, hdr);
    
    return ret;
}

Int32 Director::send(NodeBase* node, CacheHdr* hdr) {
    Bool bOk = TRUE;

    bOk = lock(node);
    if (bOk) {
        CacheCenter::queue(hdr, &node->m_snd_que_tmp);
        
        unlock(node);

        m_sender->addTask(&node->m_snd_task, BIT_EVENT_NORM);

        return 0;
    } else {
        CacheCenter::free(hdr);

        return -1;
    }
}

Int32 Director::direct(NodeBase* node, CacheHdr* hdr) {
    Bool bOk = TRUE;

    bOk = lock(node);
    if (bOk) {
        CacheCenter::queue(hdr, &node->m_cmd_que_tmp);
        
        unlock(node);

        addTask(&node->m_cmd_task, BIT_EVENT_NORM);

        return 0;
    } else {
        CacheCenter::free(hdr);

        return -1;
    }
}

Void Director::procRegistCmd(NodeBase* node, CacheHdr* hdr) {
    Int32 ev = 0; 

    ev = (Int32)getExchVal(hdr);

    addQue(node);
    if (EVENT_TYPE_RD & ev) {
        recvExch(ENUM_MSG_ADD_FD, (Int64)node);
    }

    if (EVENT_TYPE_WR & ev) {
        sendExch(&m_ctrl_node->m_base, ENUM_MSG_ADD_FD, (Int64)node);
    }

    CacheCenter::free(hdr);
}

Void Director::procCloseCmd(NodeBase* node, CacheHdr* hdr) {
    Int32 errcode = 0;

    errcode = (Int32)getExchVal(hdr);
    if (!node->m_reason) {
        LOG_INFO("close_node| fd=%d| errcode=%d|",
            node->getFd(node), errcode);
        
        node->m_reason = errcode;
        node->m_fd_status = ENUM_FD_CLOSING;

        /* start to del from receiver */
        recvExch(ENUM_MSG_DEL_FD, (Int64)node);
    } else {
        LOG_INFO("close_node| fd=%d| errcode=%d| last_err=%d|",
            node->getFd(node), errcode, node->m_reason); 
    }

    CacheCenter::free(hdr);
}

Void Director::procStopCmd(NodeBase* node, CacheHdr* hdr) {
    Int32 status = 0;

    status = (Int32)getExchVal(hdr); 
    if (ENUM_FD_STOP_RCV == status) {
        node->m_fd_status = status; 

        /* stop from dealer */ 
        dispatchExch(node, ENUM_MSG_TYPE_STOP, ENUM_FD_STOP_DEAL);
    } else if (ENUM_FD_STOP_DEAL == status) { 
        node->m_fd_status = status;

        /* del from sender */
        sendExch(&m_ctrl_node->m_base, ENUM_MSG_DEL_FD, (Int64)node);
    } else if (ENUM_FD_STOP_SND == status) {  
        node->m_fd_status = status;
        endTask(&node->m_cmd_task);
    } else {
        /* invalid */
    } 

    CacheCenter::free(hdr);
}

Void Director::dealCmd(NodeBase* node, CacheHdr* hdr) {
    if (MsgCenter::chkSysMsg(hdr)) {
        dealSysMsg(node, hdr);
    } else {
        node->dealCmd(node, hdr);
    }
}

Void Director::dealSysMsg(NodeBase* node, CacheHdr* hdr) {
    Int32 type = CacheCenter::getType(hdr);
    
    if (ENUM_MSG_TIMER_TICK == type) {
        procTimer(CTRL_TYPE_CMD, hdr); 
    } else if (ENUM_MSG_TYPE_STOP == type) {
        procStopCmd(node, hdr);
    } else if (ENUM_MSG_TYPE_CLOSE == type) {
        procCloseCmd(node, hdr);
    } else if (ENUM_MSG_REGIST_TASK == type) {
        procRegistCmd(node, hdr);
    } else {
        CacheCenter::free(hdr);
    }
}

Void Director::dealCtrlRcv(CacheHdr* hdr) {
    Int32 type = CacheCenter::getType(hdr);
    NodeBase* node = NULL; 

    node = (NodeBase*)getExchVal(hdr);
    
    if (ENUM_MSG_ADD_FD == type) {
        m_recver->addNode(node);
    } else if (ENUM_MSG_DEL_FD == type) {
        m_recver->endTask(&node->m_rcv_task);
    } else {
    }

    CacheCenter::free(hdr);
}

Void Director::dealCtrlSnd(CacheHdr* hdr) {
    Int32 type = CacheCenter::getType(hdr);
    NodeBase* node = NULL;
    
    node = (NodeBase*)getExchVal(hdr);
    
    if (ENUM_MSG_ADD_FD == type) { 
        m_sender->addNode(node);
    } else if (ENUM_MSG_DEL_FD == type) { 
        m_sender->endTask(&node->m_snd_task);
    } else {
    }

    CacheCenter::free(hdr);
}

void Director::addTimer(EnumCtrlType ctype, struct TimerEle* ele, 
    Int32 type, Uint32 interval) {
    ele->m_type = type;
    ele->m_interval = interval;
    
    m_timer_node->addTimer(m_timer_node, ctype, ele);
}

void Director::stopTimer(EnumCtrlType ctype) {
    m_timer_node->stopTimer(m_timer_node, ctype);
}

Void Director::procTimer(EnumCtrlType type, CacheHdr* hdr) {
    Int32 msg_type = CacheCenter::getType(hdr);
    Uint32 cnt = 0;
    
    if (ENUM_MSG_TIMER_TICK == msg_type) {
        cnt = (Uint32)getExchVal(hdr);

        m_timer_node->doTick(m_timer_node, type, cnt);
    } else {
    }
    
    CacheCenter::free(hdr);
}

template<>
Void Director::procTimeout<CTRL_TYPE_RCV>(struct TimerEle* ele) {
    LOG_VERB("proc_recv_timeout| type=%d| interval=%u| expires=%u|",
        ele->m_type, ele->m_interval, ele->m_expires);

    updateTimer(ele);
}

template<>
Void Director::procTimeout<CTRL_TYPE_SND>(struct TimerEle* ele) {
    LOG_VERB("proc_send_timeout| type=%d| interval=%u| expires=%u|",
        ele->m_type, ele->m_interval, ele->m_expires);

    updateTimer(ele);
}

template<>
Void Director::procTimeout<CTRL_TYPE_DEAL>(struct TimerEle* ele) {
    LOG_VERB("proc_deal_timeout| type=%d| interval=%u| expires=%u|", ele->m_type, ele->m_interval, ele->m_expires);

    updateTimer(ele);
}

template<>
Void Director::procTimeout<CTRL_TYPE_CMD>(struct TimerEle* ele) {
    LOG_VERB("proc_cmd_timeout| type=%d| interval=%u| expires=%u|",
        ele->m_type, ele->m_interval, ele->m_expires);

    updateTimer(ele);
}

Void Director::addQue(NodeBase* node) { 
    list_add_back(&node->m_cmd_node, &m_cmd_list); 
}

Void Director::delQue(NodeBase* node) {
    list_del(&node->m_cmd_node, &m_cmd_list);
}

Void Director::closeTask(NodeBase* node, Int32 errcode) {
    directExch(node, ENUM_MSG_TYPE_CLOSE, errcode);
}

Void Director::registTask(NodeBase* node, Int32 ev) {
    directExch(node, ENUM_MSG_REGIST_TASK, ev);
}

Void Director::freeNodes() {
    list_node* pos = NULL;
    list_node* n = NULL;
    NodeBase* node = NULL;
    list_head* list = &m_cmd_list;

    if (!list_empty(list)) {
        list_for_each_safe(pos, n, list) {
            list_del(pos, list);

            node = list_entry(pos, NodeBase, m_cmd_node);
            
            node->destroy(node);
        }
    }
}

Int32 Director::addListener(EnumListenerType type,
    const Char szip[], Int32 port) {
    Int32 ret = 0;
    Int32 fd = -1;
    ListenerNode* node = NULL; 
    TcpParam param;

    ret = buildParam(szip, port, &param);
    if (0 != ret) { 
        LOG_ERROR("****add_listener| addr=%s:%d| ret=%d| msg=parse error|",
            szip, port, ret);

        return ret;
    }

    fd = creatTcpSrv(&param);
    if (0 > fd) {
        LOG_ERROR("****add_listener| addr=%s:%d| ret=%d| msg=error|",
            param.m_ip, param.m_port, ret);
        
        return -1;
    } 

    switch (type) {
    case ENUM_LISTENER_RTMP:
        node = creatRtmpListener(fd, this);
        break;

    case ENUM_LISTENER_SOCK:
        node = creatSockListener(fd, this);
        break;

    default:
        closeHd(fd);
        return -1;
        break;
    } 
    
    registTask(&node->m_base, EVENT_TYPE_RD);

    LOG_INFO("++++add_listener| type=%d| fd=%d| addr=%s:%d| msg=ok|",
        type, fd, param.m_ip, param.m_port);

    return 0;
}

