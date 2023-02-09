#ifndef __SOCKUTIL_H__
#define __SOCKUTIL_H__
#include"globaltype.h"


struct TcpParam;
struct iovec;

Int32 creatTcpSrv(const TcpParam* param); 

Int32 buildParam(const Char ip[], Int32 port, TcpParam* param);
Int32 getLocalInfo(Int32 fd, int* port, Char ip[], Int32 maxLen);
Int32 getPeerInfo(Int32 fd, int* port, Char ip[], Int32 maxLen);

Int32 closeHd(Int32 fd);

Int32 sendTcp(Int32 fd, const Void* buf, Int32 len);
Int32 recvTcp(Int32 fd, Void* buf, Int32 maxlen);
Int32 peekTcp(Int32 fd, Void* buf, Int32 maxlen);
Int32 acceptCli(Int32 listener);

int recvVec(int fd, struct iovec* iov, int size, int maxlen);
int sendVec(int fd, struct iovec* iov, int size, int maxlen);

Int32 creatTimerFd(Int32 ms);
Int32 creatEventFd();

Int32 writeEvent(int fd);
Int32 readEvent(Int32 fd, Uint32* pVal = NULL);

Void waitEvent(Int32 fd, Int32 millsec);

#endif

