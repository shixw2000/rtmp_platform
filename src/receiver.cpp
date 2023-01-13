#include<poll.h>
#include"sockutil.h"
#include"receiver.h"
#include"director.h"
#include"objcenter.h"
#include"msgtype.h"


Receiver::Receiver(Int32 capacity, Director* director)
    : m_capacity(capacity), m_director(director) {
    m_fds = NULL;
    m_nodes = NULL;

    INIT_LIST_HEAD(&m_run_list);

    m_event_fd = -1;
}

Receiver::~Receiver() {
}

int Receiver::init() {
    int ret = 0;

    ret = TaskThread::init();
    if (0 != ret) {
        return ret;
    }

    setName(m_director->getThrName(CTRL_TYPE_RCV));

    m_event_fd = creatEventFd();
    if (0 > m_event_fd) {
        return -1;
    }

    ARR_NEW(struct pollfd, m_capacity, m_fds);
    memset(m_fds, 0, sizeof(struct pollfd) * m_capacity); 

    ARR_NEW(NodeBase*, m_capacity, m_nodes);
    memset(m_nodes, 0, sizeof(NodeBase*) * m_capacity); 

    return 0;
}

void Receiver::finish() { 
    if (NULL != m_fds) { 
        ARR_FREE(m_fds);
    }

    if (NULL != m_nodes) { 
        ARR_FREE(m_nodes);
    }

    if (0 <= m_event_fd) {
        closeHd(m_event_fd);

        m_event_fd = -1;
    }
    
    TaskThread::finish();
}

int Receiver::setup() { 
    return 0;
}

void Receiver::teardown() {
    m_director->stopTimer(CTRL_TYPE_RCV);

    clearRunlist();
    
    /* if exit, then stop all threads */
    m_director->stop();
}

void Receiver::wait() { 
    waitEvent(DEF_POLL_WAIT_MILLISEC);
} 

Void Receiver::alarm() {
    writeEvent(m_event_fd);
}

Void Receiver::addNode(NodeBase* node) { 
    list_add_back(&node->m_rcv_node, &m_run_list); 
}

Void Receiver::delNode(NodeBase* node) {
    list_del(&node->m_rcv_node, &m_run_list);
}

Void Receiver::closeTask(NodeBase* node, Int32 errcode) {       
    delNode(node);
    m_director->closeTask(node, errcode);
}

Int32 Receiver::waitEvent(Int32 timeout) { 
    static const Int32 ERR_FLAG = POLLERR | POLLHUP | POLLNVAL; 
    list_node* pos = NULL;
    struct pollfd* pfd = NULL;
    NodeBase* node = NULL;
    Uint32 val = 0;
    Int32 cnt = 0;
    Int32 ret = 0;

    /* add event fd */
    m_fds[0].fd = m_event_fd;
    m_fds[0].events = POLLIN; 
    m_fds[0].revents = 0;
    ++cnt;
  
    list_for_each(pos, &m_run_list) {
        node = list_entry(pos, NodeBase, m_rcv_node);

        if (!node->m_can_rd) { 
            pfd = &m_fds[cnt]; 
            m_nodes[cnt] = node; 
            ++cnt;
            
            pfd->fd = node->getFd(node);
            pfd->events = POLLIN; 
            pfd->revents = 0;
        } else {
            continue;
        }
    }

    ret = poll(m_fds, cnt, timeout);
    if (0 < ret) {  
        /* check event fd */
        if (POLLIN & m_fds[0].revents) {
            readEvent(m_event_fd, &val);
            
            --ret;
        } 
        
        for (int i=1; i<cnt && 0 < ret; ++i) {
            pfd = &m_fds[i]; 
            node = m_nodes[i]; 

            if (POLLIN & pfd->revents) {
                node->m_can_rd = TRUE;
                
                /* this branch falls in with event_fd, timer_fd */
                addTask(&node->m_rcv_task, BIT_EVENT_READ);

                --ret;
            } else if (ERR_FLAG & pfd->revents) { 
                LOG_INFO("chk_event_poll| fd=%d| revent=0x%x|"
                    " error=poll error|",
                    pfd->fd, pfd->revents); 

                closeTask(node, ENUM_ERR_SOCK_CHK_IO);

                --ret;
            } else {
                continue;
            }
        }
    } else if (0 == ret || EINTR == errno) {
        ret = 0;
    } else {
        ret = -1;
    }

    return ret;
}

Uint32 Receiver::procTask(struct Task* task) {
    Uint32 ret = 0;
    NodeBase* node = NULL;

    node = list_entry(task, NodeBase, m_rcv_task);

    ret = readNode(node);
    return ret;
}

void Receiver::procTaskEnd(struct Task* task) {
    NodeBase* node = NULL;

    node = list_entry(task, NodeBase, m_rcv_task);

    delNode(node);
    m_director->directExch(node, ENUM_MSG_TYPE_STOP, ENUM_FD_STOP_RCV);
}

Uint32 Receiver::readNode(NodeBase* node) {
    EnumSockRet ret = ENUM_SOCK_MARK_FINISH;

    ret = node->readNode(node);
    if (ENUM_SOCK_MARK_FINISH == ret) {
        node->m_can_rd = FALSE;
        
        return BIT_EVENT_NORM;
    } else if (ENUM_SOCK_MARK_PARTIAL == ret) {
        return BIT_EVENT_IN;
    } else {
        closeTask(node, ENUM_ERR_SOCK_DATA_RD);
        return BIT_EVENT_ERROR;
    }
}

Void Receiver::clearRunlist() {
    list_node* pos = NULL;
    list_node* n = NULL;
    list_head* list = &m_run_list;

    if (!list_empty(list)) {
        list_for_each_safe(pos, n, list) {
            list_del(pos, list);
        }
    }
}

