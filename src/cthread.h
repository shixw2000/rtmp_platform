#ifndef __CTHREAD_H__
#define __CTHREAD_H__
#include"sysoper.h"


class CThread {
    struct _intern;
    
public:
    CThread();
    virtual ~CThread();

    Uint64 getThr() const;

    void setName(const char name[]);

    int start();
    
    void join(); 

protected:
    virtual void run() = 0;

    virtual int setup() { return 0; }
    virtual void teardown() {}

private:
    static void* activate(void* arg);
    
private:
    struct _intern* m_intern;
};


#endif

