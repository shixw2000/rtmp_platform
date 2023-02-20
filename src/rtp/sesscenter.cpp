#include<string.h>
#include"sesscenter.h"


Session* SessCenter::findSess(const Char* sid) {
    typeSessItr itr;

    itr = m_mapSession.find(sid);
    if (m_mapSession.end() != itr) {
        return itr->second;
    } else {
        return NULL;
    }
}

Void SessCenter::addSess(Session* sess) {
    typeSessItr itr;

    if (DEF_NULL_CHAR != sess->m_sid[0]) {
        itr = m_mapSession.find(sess->m_sid);
        if (m_mapSession.end() == itr) {
            m_mapSession[sess->m_sid] = sess;
        } 
    }
}

Void SessCenter::delSess(const Char* sid) {
    typeSessItr itr;

    itr = m_mapSession.find(sid);
    if (m_mapSession.end() != itr) {
        m_mapSession.erase(itr);
    } 
}

TransInfo* SessCenter::findTrans(const IpInfo* addr) {
    typeTransItr itr;

    itr = m_mapTrans.find(addr);
    if (m_mapTrans.end() != itr) {
        return itr->second;
    } else {
        return NULL;
    }
}

Int32 SessCenter::addTrans(TransInfo* trans) {
    typeTransItr itrRtp;
    typeTransItr itrRtcp;

    itrRtp = m_mapTrans.find(&trans->m_peer_ip[ENUM_RTSP_STREAM_RTP]);
    if (m_mapTrans.end() == itrRtp) {
        itrRtcp = m_mapTrans.find(&trans->m_peer_ip[ENUM_RTSP_STREAM_RTCP]);
        if (m_mapTrans.end() == itrRtcp) {
            m_mapTrans[&trans->m_peer_ip[ENUM_RTSP_STREAM_RTP] ] = trans;
            m_mapTrans[&trans->m_peer_ip[ENUM_RTSP_STREAM_RTCP] ] = trans;

            return 0;
        } else {
            return -2;
        }
    } else {
        return -1;
    }
}

Void SessCenter::delTrans(TransInfo* trans) {
    typeTransItr itrRtp;
    typeTransItr itrRtcp;

    itrRtp = m_mapTrans.find(&trans->m_peer_ip[ENUM_RTSP_STREAM_RTP]);
    if (m_mapTrans.end() != itrRtp) {
        m_mapTrans.erase(itrRtp);
    }

    itrRtcp = m_mapTrans.find(&trans->m_peer_ip[ENUM_RTSP_STREAM_RTCP]);
    if (m_mapTrans.end() != itrRtcp) {
        m_mapTrans.erase(itrRtcp);
    }
}

Bool SessCenter::isRtp(const TransInfo* trans, const IpInfo* addr) {
    return addr->m_port == trans->m_peer_ip[ENUM_RTSP_STREAM_RTP].m_port;
}

Session* SessTool::creatSess() {
    Session* sess = NULL;

    I_NEW(Session, sess);

    resetSess(sess);
    
    return sess;
}

Void SessTool::freeSess(Session* sess) {
    if (NULL != sess) {
        resetSess(sess);
        
        I_FREE(sess);
    }
}

Void SessTool::resetSess(Session* sess) {
    memset(sess, 0, sizeof(*sess));

    for (int i=0; i<MAX_PRESENT_MADIA_CNT; ++i) {
        resetTrans(&sess->m_trans[i]);
    }
}

Void SessTool::resetTrans(TransInfo* trans) {
    memset(trans, 0, sizeof(*trans));
    
    trans->m_stat = ENUM_OBJ_STAT_END;
}


