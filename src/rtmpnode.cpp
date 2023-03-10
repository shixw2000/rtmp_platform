#include"rtmpnode.h"
#include"objcenter.h"
#include"director.h"
#include"cache.h"
#include"sockutil.h"
#include"msgtype.h"
#include"analyser.h"
#include"payloads/handshake.h"
#include"payloads/payload.h"
#include"payloads/invokepld.h"
#include"payloads/streamcenter.h"


struct RtmpNodePriv {
    RtmpNode m_pub;

    Director* m_director;
    ObjCenter* m_center;
    RtmpHandler* m_handler;
    
    Rtmp m_rtmp; 
};

static Int32 parse(NodeBase* node, Byte* data, Int32 len) {
    Int32 ret = 0;
    RtmpNodePriv* _this = (RtmpNodePriv*)node;

    ret = _this->m_pub.parsePkg(&_this->m_pub, data, len);
    return ret;
}

static ChnOutput* creatChnOut(Uint32 cid) {
    ChnOutput* chn = NULL;

    I_NEW(ChnOutput, chn);
    CacheCenter::zero(chn, sizeof(ChnOutput));
    
    chn->m_cid = cid;
    return chn;
}

static Void freeChnOut(ChnOutput* chn) {
    if (NULL != chn) {
        I_FREE(chn);
    }
}

ChnInput* creatChnIn(Uint32 cid) {
    ChnInput* chn = NULL;

    I_NEW(ChnInput, chn);
    CacheCenter::zero(chn, sizeof(ChnInput));
    
    chn->m_cid = cid;
    return chn;
}

Void freeChnIn(ChnInput* chn) {
    if (NULL != chn) {
        if (NULL != chn->m_pkg) {
            CacheCenter::del(chn->m_pkg);

            chn->m_pkg = NULL;
        }
        
        I_FREE(chn);
    }
}

static Void genHeader(Uint32 sid, Uint32 delta_ts, RtmpPkg* pkg,
    HeaderRtmp* header) { 
    CacheCenter::zero(header, sizeof(HeaderRtmp)); 

    header->m_epoch = pkg->m_epoch;
    
    /* adjust payload size for stripped */
    header->m_pld_size = pkg->m_size - pkg->m_skip;
    header->m_rtmp_type = pkg->m_rtmp_type;

    /* change stream id */
    header->m_sid = sid;

    /* adust cid by msg_type and sid */
    header->m_cid = toCid(pkg->m_msg_type, sid);
    
    if (0 < delta_ts && chkFlvData(pkg->m_msg_type)) {
        header->m_epoch += delta_ts;
    }

    /* fmt and timestamp are set by adjust header */ 
}

static Void adjustHeader(HeaderRtmp* header, Rtmp* rtmp) {
    Uint32 cid = header->m_cid;
    ChnOutput* chn = NULL;

    do { 
        if (NULL != rtmp->m_chnnOut[cid]) {
            chn = rtmp->m_chnnOut[cid];
        } else {
            /* the first time of this channel */
            chn = creatChnOut(cid);
            
            rtmp->m_chnnOut[cid] = chn;

            header->m_timestamp = header->m_epoch;
            header->m_fmt = RTMP_LARGE_FMT;
            break;
        } 
        
        /* set timestamp */
        if (header->m_sid == chn->m_sid && header->m_epoch >= chn->m_epoch) {
            header->m_timestamp = header->m_epoch - chn->m_epoch;
        } else { 
            /* if sid has changed, or the timestamp go down, then set fmt large */
            header->m_timestamp = header->m_epoch;
            header->m_fmt = RTMP_LARGE_FMT;
            break;
        }

        if (header->m_pld_size == chn->m_pld_size
            && header->m_rtmp_type == chn->m_rtmp_type) {
            if (header->m_timestamp == chn->m_timestamp) {
                header->m_fmt = RTMP_MINIMUM_FMT;
            } else {
                header->m_fmt = RTMP_SMALL_FMT;
            }
        } else {
            header->m_fmt = RTMP_MEDIUM_FMT;
        }
    } while (0);

    /* updata chn */
    chn->m_epoch = header->m_epoch;
    chn->m_timestamp = header->m_timestamp;
    chn->m_pld_size = header->m_pld_size;
    chn->m_rtmp_type = header->m_rtmp_type;
    chn->m_sid = header->m_sid;
}

static Int32 buildHeader(HeaderRtmp* header, Byte buff[], Int32 size) {
    Uint32 pldSize = header->m_pld_size;
    Uint32 ts = 0;
    Byte type = header->m_rtmp_type;
    Byte ch = 0;
    Builder builder(buff, size);

    if (RTMP_TIMESTAMP_EXT_MARK > header->m_timestamp) {
        ts = header->m_timestamp;
    } else {
        ts = RTMP_TIMESTAMP_EXT_MARK;
    }

    ch = (header->m_fmt << 6) | (header->m_cid & 0x3F);
    builder.build8(&ch);

    switch (header->m_fmt) {
    case RTMP_LARGE_FMT:
        builder.build24(&ts);
        builder.build24(&pldSize);
        builder.build8(&type);
        builder.build32LE(&header->m_sid);
        break;

    case RTMP_MEDIUM_FMT:
        builder.build24(&ts);
        builder.build24(&pldSize);
        builder.build8(&type);
        break;
        
    case RTMP_SMALL_FMT:
        builder.build24(&ts);
        break;

    case RTMP_MINIMUM_FMT:
    default:
        /* fmt == 3, no ext time */
        return builder.used();
        break;
    }

    if (RTMP_TIMESTAMP_EXT_MARK == ts) {
        builder.build32(&header->m_timestamp);
    }

    return builder.used();
}

static Int32 sendData(RtmpNodePriv* _this, Byte* data1, Int32 data1Len,
    Byte* data2, Int32 data2Len, Cache* cache) {
    Int32 ret = 0;
    CacheHdr* hdr = NULL;
    Rtmp* rtmp = &_this->m_rtmp; 
    
    hdr = MsgCenter::creatSendMsg(data1, data1Len, data2, data2Len, cache);
    if (NULL != hdr) { 
        ret = _this->m_director->send(&_this->m_pub.m_base, hdr); 
        if (0 == ret) {
            /* add send bytes */
            rtmp->m_snd_total_bytes += data1Len + data2Len;
        }
    } else {
        ret = -1;
    }
    
    return ret;
}

static Int32 sendData2(RtmpNodePriv* _this, Byte* data1, Int32 data1Len,
    Byte* data2, Int32 data2Len) {
    Int32 ret = 0;
    CacheHdr* hdr = NULL;
    Rtmp* rtmp = &_this->m_rtmp; 
    
    hdr = MsgCenter::creat2SendMsg(data1, data1Len, data2, data2Len);
    if (NULL != hdr) { 
        ret = _this->m_director->send(&_this->m_pub.m_base, hdr); 
        if (0 == ret) {
            /* add send bytes */
            rtmp->m_snd_total_bytes += data1Len + data2Len;
        }
    } else {
        ret = -1;
    }
    
    return ret;
}

METHOD(NodeBase, getFd, Int32, RtmpNodePriv* _this) {
    return _this->m_rtmp.m_fd;
}

METHOD(NodeBase, readNode, EnumSockRet, RtmpNodePriv* _this) {
	EnumSockRet ret = ENUM_SOCK_MARK_FINISH;

    ret = _this->m_center->readSock(&_this->m_pub.m_base, &parse);

    return ret;
}

METHOD(NodeBase, writeNode, EnumSockRet, RtmpNodePriv* _this) {
    EnumSockRet ret = ENUM_SOCK_MARK_FINISH;

    ret = _this->m_center->writeSock(&_this->m_pub.m_base);

    return ret;
}

METHOD(NodeBase, dealMsg, Int32, RtmpNodePriv* _this, CacheHdr* hdr) {
    Int32 ret = 0;

    ret = _this->m_handler->dealRtmp(&_this->m_rtmp, hdr);
    return ret;
}

METHOD(NodeBase, dealCmd, Void, RtmpNodePriv* , CacheHdr* ) {
    
    return;
}

METHOD(NodeBase, onClose, Void, RtmpNodePriv* _this) {
    _this->m_handler->closeRtmp(&_this->m_rtmp);
    
    _this->m_director->notifyExit(&_this->m_pub.m_base);
}

static Void finishRtmp(Rtmp* rtmp);

METHOD(NodeBase, destroy, Void, RtmpNodePriv* _this) { 
    ObjCenter::finishNode(&_this->m_pub.m_base); 

    finishRtmp(&_this->m_rtmp);

    I_FREE(_this);
}

METHOD(RtmpNode, parsePkg, Int32, RtmpNodePriv* _this,
    Byte* data, Int32 len) {
    Int32 ret = 0;

    _this->m_rtmp.m_rcv_total_bytes += len;
    if (0 < _this->m_rtmp.m_srv_window_size 
        && _this->m_rtmp.m_srv_window_size + _this->m_rtmp.m_rcv_ack_bytes 
            <= _this->m_rtmp.m_rcv_total_bytes) {
        _this->m_handler->sendBytesReport(&_this->m_rtmp,
            _this->m_rtmp.m_rcv_total_bytes);

        /* update recv ack bytes */
        _this->m_rtmp.m_rcv_ack_bytes = _this->m_rtmp.m_rcv_total_bytes;
    }
    
    ret = _this->m_center->parseRtmp(&_this->m_rtmp, data, len);
    
    return ret;
}

METHOD(RtmpNode, onAccept, Int32, RtmpNodePriv* ) {
    return 0;
}

METHOD(RtmpNode, onConnOk, Int32, RtmpNodePriv* ) {
    return 0;
}

METHOD(RtmpNode, onConnFail, Void, RtmpNodePriv* ) {
    return;
}

METHOD(RtmpNode, sendHandshake, Int32, RtmpNodePriv* _this, Cache* cache) {
    Int32 ret = 0;
    CommPkg* pkg = NULL; 
    
    pkg = MsgCenter::cast<CommPkg>(cache);
    
    ret = sendData(_this, NULL, 0, pkg->m_data, pkg->m_size, cache); 
    return ret;
}

METHOD(RtmpNode, sendPayload, Int32, RtmpNodePriv* _this,
    Uint32 sid, Uint32 rtmp_type, const Chunk* chunk) {
    Int32 ret = 0;
    Int32 hdr_size = 0;
    Int32 len = 0;
    Int32 payload_size = 0;
    Uint32 msg_type = 0;
    Byte* payload = NULL;
    Rtmp* rtmp = &_this->m_rtmp;
    HeaderRtmp header;
    Byte ch = 0; // used by fmt = 0x3
    Byte buff[RTMP_MAX_HEADER_SIZE] = {0};

    payload = chunk->m_data;
    payload_size = chunk->m_size;
    msg_type = toMsgType(rtmp_type);
    
    CacheCenter::zero(&header, sizeof(header)); 

    /* times are all zeros */
    header.m_pld_size = payload_size;
    header.m_rtmp_type = rtmp_type; 
    header.m_sid = sid; 
    header.m_cid = toCid(msg_type, sid);
    
    adjustHeader(&header, rtmp);
    
    /* creat first chunk header */
    hdr_size = buildHeader(&header, buff, RTMP_MAX_HEADER_SIZE);

    /* calc next chunk header */
    ch = (RTMP_MINIMUM_FMT << 6) | (header.m_cid & 0x3F); 

    while (0 < payload_size) {
        if (payload_size < rtmp->m_chunkSizeOut) {
            len = payload_size;
        } else {
            len = rtmp->m_chunkSizeOut;
        }
        
        ret = sendData2(_this, buff, hdr_size, payload, len);
        if (0 == ret) {
            payload += len;
            payload_size -= len;

            /* set next chunk header */
            buff[0] = ch;
            hdr_size = 1;
        } else {
            break;
        }
    }

    LOG_INFO("send_payload| ret=%d| user_id=%u| fd=%d| fmt=%u| cid=%u|"
        " epoch=%u| timestamp=%u|"
        " rtmp_type=%u| payload_size=%u|"
        " sid=%u| chunk_size_out=%d|",
        ret, rtmp->m_user_id,
        rtmp->m_fd, 
        header.m_fmt,
        header.m_cid, 

        header.m_epoch,
        header.m_timestamp,
        header.m_rtmp_type,
        header.m_pld_size,
        
        header.m_sid, 
        rtmp->m_chunkSizeOut);

    return ret;
}

METHOD(RtmpNode, sendPkg, Int32, RtmpNodePriv* _this, 
    Uint32 sid, Uint32 delta_ts, Cache* cache) {
    Int32 ret = 0;
    Int32 hdr_size = 0;
    Int32 len = 0;
    Int32 payload_size = 0;
    Bool is_flv = FALSE;
    Byte* payload = NULL;
    RtmpPkg* pkg = NULL;
    Rtmp* rtmp = &_this->m_rtmp;
    HeaderRtmp header;
    Byte ch = 0; // used by fmt = 0x3
    Byte buff[RTMP_MAX_HEADER_SIZE] = {0};

    pkg = MsgCenter::cast<RtmpPkg>(cache); 
    
    genHeader(sid, delta_ts, pkg, &header);
    
    adjustHeader(&header, rtmp);
    
    /* creat first chunk header */
    hdr_size = buildHeader(&header, buff, RTMP_MAX_HEADER_SIZE);
    
    /* calc next chunk header */
    ch = (RTMP_MINIMUM_FMT << 6) | (header.m_cid & 0x3F); 

    /* set begin cache */
    payload = pkg->m_payload + pkg->m_skip;
    payload_size = pkg->m_size - pkg->m_skip;
    
    while (0 < payload_size) { 
        if (payload_size < rtmp->m_chunkSizeOut) {
            len = payload_size;
        } else {
            len = rtmp->m_chunkSizeOut;
        }
        
        ret = sendData(_this, buff, hdr_size, payload, len, cache);
        if (0 == ret) {
            payload += len;
            payload_size -= len;

            /* set next chunk header */
            buff[0] = ch;
            hdr_size = 1;
        } else {
            break;
        }
    }

    is_flv = chkFlvData(pkg->m_msg_type);
    if (!is_flv || g_conf.m_prnt_flv) {
        LOG_INFO("send_pkg| ret=%d| user_id=%u| fd=%d| fmt=%u| cid=%u|"
            " epoch=%u| timestamp=%u| delta_ts=%u|"
            " rtmp_type=%u| payload_size=%u|"
            " sid=%u| pkg_sid=%u|"
            " chunk_size_out=%d|",
            ret, rtmp->m_user_id,
            rtmp->m_fd, 
            header.m_fmt,
            header.m_cid, 

            header.m_epoch,
            header.m_timestamp,
            delta_ts,
            
            header.m_rtmp_type,
            header.m_pld_size,
            
            header.m_sid, 
            pkg->m_sid,  
            rtmp->m_chunkSizeOut);
        }
    
    return ret;
}

METHOD(RtmpNode, recvCmd, Int32, RtmpNodePriv* _this, 
    Int32 msg_type, Int64 val) {
    Int32 ret = 0; 
    CacheHdr* hdr = NULL;

    hdr = MsgCenter::creatExchMsg(msg_type, val); 

    ret = _this->m_director->dispatch(&_this->m_pub.m_base, hdr); 
    return ret;
}

METHOD(RtmpNode, recv, Int32, RtmpNodePriv* _this, 
    Int32 msg_type, Uint32 sid, Cache* cache) {
    Int32 ret = 0;
    CacheHdr* hdr = NULL;
    MsgComm* msg = NULL;
    RtmpPkg* pkg = NULL;
    Rtmp* rtmp = &_this->m_rtmp;

    if (ENUM_MSG_RTMP_TYPE_CTRL != msg_type) { 
        hdr = MsgCenter::creatMsg<MsgComm>(msg_type);
        msg = MsgCenter::body<MsgComm>(hdr);

        msg->m_sid = sid; 
        
        if (ENUM_MSG_RTMP_TYPE_UNKNOWN != msg_type) { 
            
            /* set cache and add ref */
            CacheCenter::setCache(hdr, CacheCenter::ref(cache));
            
            ret = _this->m_director->dispatch(&_this->m_pub.m_base, hdr); 
        } else {
            pkg = MsgCenter::cast<RtmpPkg>(cache); 
            
            LOG_WARN("recv_msg| user_id=%u| fd=%d|"
                " epoch=%u| timestamp=%u|"
                " rtmp_type=%u| payload_size=%u|"
                " sid=%u| pkg_sid=%u|"
                " chunk_size_in=%d|",
                rtmp->m_user_id,
                rtmp->m_fd, 

                pkg->m_epoch,
                pkg->m_timestamp,
                pkg->m_rtmp_type,
                pkg->m_size,
                
                sid, 
                pkg->m_sid,  
                rtmp->m_chunkSizeIn);

            ret = -1;
        }
    } else {
        ret = _this->m_handler->dealCtrlMsg(&_this->m_rtmp, cache); 
    } 
    
    return ret;
}

static Void initRtmp(Rtmp* rtmp) {
    CacheCenter::zero(rtmp, sizeof(Rtmp)); 
    
    INIT_LIST_HEAD(&rtmp->m_rpc_calls);
    rtmp->m_fd = -1;
    rtmp->m_chunkSizeIn = RTMP_MIN_CHUNK_SIZE;
    rtmp->m_chunkSizeOut = RTMP_MIN_CHUNK_SIZE;
    rtmp->m_status = ENUM_RTMP_STATUS_INIT;
    
    rtmp->m_chn.m_rd_stat = ENUM_RTMP_RD_INIT;
}

static Void finishRtmp(Rtmp* rtmp) {
    freeAV(&rtmp->m_app);
    freeAV(&rtmp->m_tcUrl); 

    if (NULL != rtmp->m_chn.m_cache) {
        CacheCenter::del(rtmp->m_chn.m_cache);
        rtmp->m_chn.m_cache = NULL;
    }
    
    for (int i=0; i<RTMP_MAX_CHNN_ID; ++i) {
        if (NULL != rtmp->m_chnnIn[i]) {
            freeChnIn(rtmp->m_chnnIn[i]);

            rtmp->m_chnnIn[i] = NULL;
        }

        if (NULL != rtmp->m_chnnOut[i]) {
            freeChnOut(rtmp->m_chnnOut[i]);

            rtmp->m_chnnOut[i] = NULL;
        }
    } 

    for (Uint32 i=0; i<RTMP_MAX_SESS_CNT; ++i) {
        if (NULL != rtmp->m_unit[i]) {
            StreamCenter::freeUnit(rtmp->m_unit[i]);

            rtmp->m_unit[i] = NULL;
        }
    }
    
    if (0 <= rtmp->m_fd) {
        closeHd(rtmp->m_fd);
        rtmp->m_fd = -1;
    }
}

Uint32 toMsgType(Uint32 rtmp_type) {
    Uint32 msg_type = ENUM_MSG_TYPE_END;
    
    if (RTMP_MSG_TYPE_AUDIO == rtmp_type) {
        msg_type = ENUM_MSG_RTMP_TYPE_AUDIO;
    } else if (RTMP_MSG_TYPE_VIDEO == rtmp_type) {
        msg_type = ENUM_MSG_RTMP_TYPE_VIDEO;
    } else if (RTMP_MSG_TYPE_INVOKE == rtmp_type) {
        msg_type = ENUM_MSG_RTMP_TYPE_INVOKE;
    } else if (RTMP_MSG_TYPE_CLIENT_BW >= rtmp_type) {
        msg_type = ENUM_MSG_RTMP_TYPE_CTRL;
    } else if (RTMP_MSG_TYPE_META_INFO == rtmp_type) {
        msg_type = ENUM_MSG_RTMP_TYPE_META_INFO;
    } else {
        msg_type = ENUM_MSG_RTMP_TYPE_UNKNOWN;
    }

    return msg_type;
}

Uint32 toCid(Uint32 msg_type, Uint32 sid) {
    Uint32 cid = 0;
 
    switch (msg_type) {
    case ENUM_MSG_RTMP_TYPE_VIDEO: 
        cid = (sid << 1) + RTMP_SOURCE_CHANNEL;
        break;
        
    case ENUM_MSG_RTMP_TYPE_AUDIO:
    case ENUM_MSG_RTMP_TYPE_META_INFO:
        cid = (sid << 1) + RTMP_OTHER_CHANNEL;
        break;

    case ENUM_MSG_RTMP_TYPE_INVOKE:
        if (0 == sid) {
            cid = RTMP_INVOKE_CID;
        } else {
            cid = RTMP_SOURCE_CHANNEL;
        }
        
        break;

    case ENUM_MSG_RTMP_TYPE_CTRL:
        cid = RTMP_CTRL_CID;
        break;

    default:
        cid = RTMP_OTHER_CHANNEL;
        break;
    } 

    return cid;
}

Bool chkFlvData(Uint32 msg_type) {
    if (ENUM_MSG_RTMP_TYPE_AUDIO == msg_type
        || ENUM_MSG_RTMP_TYPE_VIDEO == msg_type
        || ENUM_MSG_RTMP_TYPE_META_INFO == msg_type) {
        return TRUE;
    } else {
        return FALSE;
    }
}

Cache* creatPkg(Uint32 rtmp_type, Uint32 payload_size) {
    Cache* cache = NULL;
    RtmpPkg* pkg = NULL;

    cache = MsgCenter::creatCache<RtmpPkg>(payload_size);
    if (NULL != cache) {
        pkg = MsgCenter::cast<RtmpPkg>(cache); 

        pkg->m_size = (Int32)payload_size;
        pkg->m_rtmp_type = rtmp_type; 
        pkg->m_msg_type = toMsgType(rtmp_type);
    }

    return cache;
}

Bool chkRtmpVer(Byte ver) {
    return RTMP_DEF_HANDSHAKE_VER == ver;
}

RtmpNode* creatRtmpNode(Int32 fd, Director* director) {
    RtmpNodePriv* _this = NULL;

    I_NEW(RtmpNodePriv, _this);
    CacheCenter::zero(_this, sizeof(RtmpNodePriv));

    ObjCenter::initNode(&_this->m_pub.m_base, ENUM_NODE_RTMP); 
    
    initRtmp(&_this->m_rtmp);
    
    _this->m_rtmp.m_fd = fd;
    _this->m_rtmp.m_entity = &_this->m_pub;
    
    _this->m_director = director;
    _this->m_center = director->getCenter(); 
    _this->m_handler = _this->m_center->getRtmpHandler();

    director->condition(&_this->m_pub.m_base.m_rcv_task, BIT_EVENT_READ);
    director->condition(&_this->m_pub.m_base.m_snd_task, BIT_EVENT_WRITE);
    
    _this->m_pub.m_base.getFd = _getFd;
    _this->m_pub.m_base.readNode = _readNode;
    _this->m_pub.m_base.writeNode = _writeNode;

    _this->m_pub.m_base.dealMsg = _dealMsg;
    _this->m_pub.m_base.dealCmd = _dealCmd;
    
    _this->m_pub.m_base.onClose = _onClose;
    _this->m_pub.m_base.destroy = _destroy;

    _this->m_pub.parsePkg = _parsePkg;
    _this->m_pub.onAccept = _onAccept;
    _this->m_pub.onConnOk = _onConnOk;
    _this->m_pub.onConnFail = _onConnFail;
    
    _this->m_pub.recv = _recv;
    _this->m_pub.recvCmd = _recvCmd;
    
    _this->m_pub.sendHandshake = _sendHandshake;
    _this->m_pub.sendPayload = _sendPayload; 
    _this->m_pub.sendPkg = _sendPkg; 
  
    return &_this->m_pub;
}


