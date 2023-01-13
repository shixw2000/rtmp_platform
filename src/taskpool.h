#ifndef __TASKPOOL_H__
#define __TASKPOOL_H__
#include"tasker.h" 
#include"cthread.h"


class Lock;

class TaskThread : public Tasker, public CThread {
public:
    TaskThread();
    virtual ~TaskThread();
    
    virtual int init();
    virtual void finish();

    virtual void stop(); 

protected:
    virtual void run();
    virtual Bool lock();
    virtual Bool unlock();

private:
    Lock* m_lock;
};

class TaskWorker : public TaskThread {
public:
    TaskWorker();
    virtual ~TaskWorker(); 

    virtual int init();
    virtual void finish(); 
  
protected: 
    virtual void wait();
    virtual void alarm(); 

private:
    int m_event_fd; 
};


#endif

