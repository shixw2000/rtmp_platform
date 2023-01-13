#include"cthread.h"
#include"lock.h"
#include"sockutil.h"
#include"taskpool.h"


TaskThread::TaskThread() {
    m_lock = NULL;
}

TaskThread::~TaskThread() {
}

int TaskThread::init() {
    int ret = 0;

    I_NEW(SpinLock, m_lock);
    ret = m_lock->init();
    if (0 != ret) {
        return ret;
    }

    return 0;
}

void TaskThread::finish() { 
    if (NULL != m_lock) {
        m_lock->finish();
        I_FREE(m_lock);
    }
}

void TaskThread::run() {
    startSvc();
}

void TaskThread::stop() {
    stopSvc();
}

Bool TaskThread::lock() {
    return m_lock->lock();
}

Bool TaskThread::unlock() {
    return m_lock->unlock();
}


TaskWorker::TaskWorker() {
    m_event_fd = -1; 
}

TaskWorker::~TaskWorker() {
}

int TaskWorker::init() {
    int ret = 0;

    ret = TaskThread::init();
    if (0 != ret) {
        return ret;
    }

    m_event_fd = creatEventFd();
    if (0 > m_event_fd) {
        return -1;
    }
    
    return 0;
}

void TaskWorker::finish() { 
    if (0 <= m_event_fd) {
        closeHd(m_event_fd);

        m_event_fd = -1;
    }

    TaskThread::finish();
}

void TaskWorker::alarm() {
    writeEvent(m_event_fd); 
}
 
void TaskWorker::wait() {
    waitEvent(m_event_fd, -1);
}


