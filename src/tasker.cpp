#include"tasker.h"


unsigned int BitTool::addBit(unsigned int* pstate, unsigned int n) {
    unsigned int oldval = 0U;
    unsigned int newval = 0U;
    unsigned int retval = 0U;
    
    do {
        oldval = retval;
        
        if (n != (oldval & n)) {
            newval = (oldval | n);
        } else {
            /* already has the flag */
            break;
        }
        
        retval = CMPXCHG(pstate, oldval, newval);
    } while (retval != oldval);

    return oldval;
}

unsigned int BitTool::setBit(unsigned int* pstate, unsigned int n) {
    unsigned int state = 0U;
    
    state = ATOMIC_SET(pstate, n);
    return state;
}

bool BitTool::isMember(unsigned int state, unsigned int n) {
    return (n == (n & state));
}

bool BitTool::casBit(unsigned int* pstate, 
    unsigned int newval, unsigned int oldval) {
    bool bOk = true;

    bOk = CAS(pstate, oldval, newval);
    return bOk;
}

BinaryDoor::BinaryDoor() {
    m_state = 0U;
}

bool BinaryDoor::login() {
    unsigned int state = 0U;

    state = BitTool::addBit(&m_state, DEF_LOGIN_FLAG);
    if (0 == state) {
        return true;
    } else {
        return false;
    }
}

void BinaryDoor::lock() {    
    (void)BitTool::setBit(&m_state, DEF_LOCK_FLAG);
}

bool BinaryDoor::release() {
    bool bOk = true; 
    
    bOk = BitTool::casBit(&m_state, 0, DEF_LOCK_FLAG); 
    return bOk;
}


Service::Service() { 
    m_running = false; 
}

Service::~Service() {
}

void Service::resume() {
    bool needFire = false;

    needFire = m_door.login();
    if (needFire && m_running) {
        alarm();
    }
}

Void Service::startSvc() { 
    bool done = true;

    m_running = true;
    
    m_door.lock();
    while (m_running) {
        done = doService();
        if (done) {
            done = m_door.release();
            if (done) {
                /* nothing to do, then wait here */
                wait();
            } 
        } 

        /* restart the door state */
        m_door.lock();
    } 
}

void Service::stopSvc() {
    if (m_running) {
        m_running = false;

        alarm();
    }
}


BitDoor::BitDoor() {
    for (int i=0; i<BIT_EVENT_MAX; ++i) {
        m_low_bits[i] = (0x1 << i);
        m_high_bits[i] = (m_low_bits[i] << BIT_EVENT_MAX);
        m_mask_bits[i] = (m_low_bits[i] | m_high_bits[i]);
    }
}

unsigned int BitDoor::lock(unsigned int* pstate) {
    unsigned int old_state = 0U;

    old_state = BitTool::setBit(pstate, m_low_bits[BIT_EVENT_LOCK]);
    return old_state;
}

bool BitDoor::release(unsigned int* pstate) {
    bool bOk = true; 
    
    bOk = BitTool::casBit(pstate, 0, m_low_bits[BIT_EVENT_LOCK]); 
    return bOk;
}

bool BitDoor::checkin(unsigned int* pstate, unsigned int ev) {    
    unsigned int old_state = 0U;
    unsigned int old_high = 0U;

    if (BIT_EVENT_NORM <= ev && BIT_EVENT_END > ev) {
        old_state = BitTool::addBit(pstate, m_low_bits[ev]);
        if (0 == old_state || (m_high_bits[ev] == (old_state & m_mask_bits[ev]))) {
            return true;
        } else {
            return false;
        }
    } else if (BIT_EVENT_END == ev) {
        /* to quick end */
        old_state = BitTool::setBit(pstate, 0xFFFF);
        if (0 == old_state) {
            return true;
        } else {
            old_high = (old_state >> BIT_EVENT_MAX);

            /* get correspending bit between high and low mask */
            if (0 != (old_high ^ (old_state & old_high))) {
                return true;
            } else {
                return false;
            }
        } 
    } else {
        /* invalid */
        return false;
    }
}

bool BitDoor::checkout(unsigned int* pstate, unsigned int ev) {
    unsigned int old_state = 0U;

    old_state = BitTool::addBit(pstate, m_high_bits[ev]);
    if (m_low_bits[ev] == (old_state & m_mask_bits[ev])) {
        return false;
    } else {
        return true;
    }
}

bool BitDoor::isEof(unsigned int state) {
    bool isEnd = false;
    
    isEnd = BitTool::isMember(state, m_low_bits[BIT_EVENT_END]);
    return isEnd;
} 

Pooler::Pooler() { 
    INIT_ROOT(&m_node_list);
    INIT_ROOT(&m_tmp_list);
}

Pooler::~Pooler() {
}

void Pooler::condition(struct Task* task, unsigned int ev) {
    if (BIT_EVENT_CUSTOM <= ev && BIT_EVENT_END > ev) {
        m_door.checkout(&task->m_state, ev);
    }
}

bool Pooler::_add(struct Task* task, unsigned int ev) {
    Bool bOk = TRUE;
    
    bOk = m_door.checkin(&task->m_state, ev);
    if (bOk) {
        bOk = lock();
        if (bOk) {
            pushNode(&m_tmp_list, &task->m_node);
            unlock(); 
        } 
    } 

    return bOk;
}

bool Pooler::endTask(struct Task* task) {
    bool added = FALSE;

    added = _add(task, BIT_EVENT_END); 
    return added;
}

bool Pooler::addTask(struct Task* task, unsigned int ev) {
    bool added = FALSE;

    if (BIT_EVENT_NORM <= ev && BIT_EVENT_END > ev) {
        added = _add(task, ev);

        return added;
    } else {
        return FALSE;
    } 
}

void Pooler::_splice() {
    bool bOk = TRUE;

    bOk = lock();
    if (bOk) {
        if (!isRootEmpty(&m_tmp_list)) {
            spliceRoot(&m_tmp_list, &m_node_list);
        }
        
        unlock();
    } 
}

bool Pooler::consume() {
    struct node* prev = NULL;
    struct node* curr = NULL;
    struct node* next = NULL;
    struct Task* task = NULL;
    unsigned int old_state = 0U;
    unsigned int ret = 0;
    bool done = FALSE;

    _splice(); 
    
    for (prev=&m_node_list.m_head, curr=prev->m_next; !!curr; curr=next) {
        next = curr->m_next;

        task = containof(curr, struct Task, m_node);
        
        old_state = m_door.lock(&task->m_state);

        ret = doTask(task, old_state);
        if (BIT_EVENT_NORM <= ret && BIT_EVENT_CUSTOM > ret) {
            done = m_door.release(&task->m_state);
        } else if (BIT_EVENT_CUSTOM <= ret && BIT_EVENT_END > ret) {
            done = m_door.checkout(&task->m_state, ret);
        } else if (BIT_EVENT_IN == ret) {
            /* 1 means to keep in the queue */
            done = false;
        } else {
            /* others mean out ot queue */
            done = true;
        }

        if (done) {
            /* erase from the queue */
            prev->m_next = next;

            --m_node_list.m_size;
        } else {
            /* keep in the queue */
            prev = curr;
        }
    }

    m_node_list.m_tail = prev;

    return isRootEmpty(&m_node_list);
}

unsigned int Pooler::doTask(struct Task* task, unsigned int oldstate) {
    unsigned int ret = 0;
    bool isEnd = FALSE;
    
    isEnd = m_door.isEof(oldstate);
    if (!isEnd) {
        ret = procTask(task);

        return ret;
    } else {
        /* the end of task */
        procTaskEnd(task);
        
        return BIT_EVENT_END;
    }
}


Bool Tasker::doService() {
    Bool done = FALSE;
    
    done = consume();
    return done;
}

bool Tasker::endTask(struct Task* task) {
    bool added = FALSE;

    added = Pooler::endTask(task);
    if (added) {
        resume();
    }
    
    return added;
}

bool Tasker::addTask(struct Task* task, unsigned int ev) {
    bool added = FALSE;

    added = Pooler::addTask(task, ev);
    if (added) {
        resume();
    }

    return added;
}


