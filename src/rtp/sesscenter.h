#ifndef __SESSCENTER_H__
#define __SESSCENTER_H__
#include"stdtype.h"
#include"sysoper.h"
#include"rtspmsg.h"
#include"resmanage.h"


enum EnumObjStat {
    ENUM_OBJ_STAT_INIT,

    ENUM_OBJ_STAT_READY,
    
    ENUM_OBJ_STAT_PLAYING,
    ENUM_OBJ_STAT_RECORDING,

    ENUM_OBJ_STAT_END
};

struct Session;
struct Presentation;

struct TransInfo {
    Session* m_parent;
    Int32 m_id; // in presentation
    Int32 m_stat;
    IpInfo m_peer_ip[ENUM_RTSP_STREAM_END];
};

struct Session {
    Presentation* m_preset; 
    TransInfo m_trans[MAX_PRESENT_MADIA_CNT]; // used stream by session
    Int32 m_trans_size; 
    Char m_sid[MAX_SESS_ID_LEN];
};

class SessTool {
public:
    static Session* creatSess();
    static Void freeSess(Session* sess);

    static Void resetSess(Session* sess);
    static Void resetTrans(TransInfo* trans);
};

class SessCenter {
public:
    Session* findSess(const Char* sid);
    Void addSess(Session* sess);
    Void delSess(const Char* sid);

    TransInfo* findTrans(const IpInfo* addr);
    Int32 addTrans(TransInfo* trans);
    Void delTrans(TransInfo* trans);

    Bool isRtp(const TransInfo* trans, const IpInfo* addr); 
    
private:
    typeMapSess m_mapSession;
    typeMapTrans m_mapTrans;
};

#endif

