#ifndef __GLOBALTYPE_H__
#define __GLOBALTYPE_H__
#include"sysoper.h"
#include"misc.h"


static const Int32 DEF_EPOLL_WAIT_MILI_SEC = 1000;
static const Int32 DEF_VEC_SIZE = 100;
static const Int32 DEF_TCP_SEND_BUF_SIZE = 0x100000;
static const Int32 DEF_TCP_RCV_BUF_SIZE = DEF_TCP_SEND_BUF_SIZE;
static const Int32 MAX_SOCKET_BUFFER_SIZE = 0x500000;
static const Int32 MAX_FD_SIZE = 0x100000;
static const Int32 MAX_MSG_SIZE = 0x500000;

static const Uint16 DEF_MSG_VER = 0x0814;
static const Byte DEF_DELIM_END_VALID = 0xAD;
static const Byte DEF_DELIM_END_INVALID = 0xAE;

static const Int32 DEF_IP_SIZE = 32;
static const Int32 DEF_ADDR_SIZE = 128; 

static const Int32 DEF_LOCK_ORDER = 3;
static const Int32 DEF_FD_MAX_CAPACITY = 200000;

static const Int32 DEF_POLL_WAIT_MILLISEC = 1000;


#endif

