#include"invokepld.h"
#include"cache.h"
#include"rtmpnode.h"
#include"analyser.h"
#include"payload.h"
#include"handshake.h"
#include"msgtype.h"
#include"streamcenter.h"
#include"rtmpproto.h"


#define F_PTR_VAL(f) (f ? *f : 0)

RtmpHandler::RtmpHandler() {
    m_handshake = NULL;
    m_amf = NULL;
    m_stream_center = NULL;
    m_proto_center = NULL;

    m_user_id = 100;
}

RtmpHandler::~RtmpHandler() {
}

Int32 RtmpHandler::init() {
    I_NEW(HandShake, m_handshake);
    I_NEW(AmfPayload, m_amf);
    I_NEW_P(RtmpProto, m_proto_center, m_amf);
    I_NEW_P(StreamCenter, m_stream_center, m_proto_center); 

    return 0;
}

Void RtmpHandler::finish() {
    I_FREE(m_handshake);
    I_FREE(m_amf);
    I_FREE(m_proto_center);
    I_FREE(m_stream_center);
}

RtmpHandler* RtmpHandler::creat() {
    Int32 ret = 0;
    RtmpHandler* hd = NULL;
    
    I_NEW(RtmpHandler, hd);
    if (NULL != hd) {
        ret = hd->init();
        if (0 != ret) {
            free(hd);

            hd = NULL;
        }
    }
    
    return hd;
}

Void RtmpHandler::free(RtmpHandler* hd) {
    if (NULL != hd) {
        hd->finish();
        I_FREE(hd);
    }
}

Int32 RtmpHandler::dealHandshakeC01(Rtmp* rtmp, CacheHdr* hdr) {
    Int32 ret = 0;
    Cache* cache = NULL;
    CommPkg* pkg = NULL;
    RtmpNode* node = rtmp->m_entity;
    RtmpHandshake* rtmpHandshake = &rtmp->m_chn.m_handshake;

    cache = CacheCenter::getCache(hdr);
    pkg = MsgCenter::cast<CommPkg>(cache);
    
    ret = m_handshake->parseC0C1(pkg->m_data, rtmpHandshake);
    if (0 == ret) {
        /* reused the cache to create s0s1 */        
        m_handshake->creatS0S1(pkg->m_data, rtmpHandshake);

        /* send c01 and this cache is free by hdr in function end */
        node->sendHandshake(node, cache);
        
        /* creat s2 */ 
        cache = MsgCenter::creatCache<CommPkg>(RTMP_SIG_SIZE);
        pkg = MsgCenter::cast<CommPkg>(cache);
        pkg->m_size = RTMP_SIG_SIZE;
        
        m_handshake->creatS2(pkg->m_data, rtmpHandshake);

        /* send s2 */
        node->sendHandshake(node, cache);
        
        /* release cache of s2 here */
        CacheCenter::del(cache);
        
        LOG_INFO("handshake| msg=c01 ok|");
    } else {
        LOG_ERROR("handshake| msg=c01 error|");

        rtmp->m_status = ENUM_RTMP_STATUS_ERROR;
    }

    return ret;
}

Int32 RtmpHandler::dealHandshakeC2(Rtmp* rtmp, CacheHdr* hdr) {
    Int32 ret = 0;
    Cache* cache = NULL;
    CommPkg* pkg = NULL;
    RtmpHandshake* rtmpHandshake = &rtmp->m_chn.m_handshake;

    cache = CacheCenter::getCache(hdr);
    pkg = MsgCenter::cast<CommPkg>(cache);
    
    ret = m_handshake->parseC2(pkg->m_data, rtmpHandshake);
    if (0 == ret) {
        LOG_INFO("handshake| msg=c2 ok|");
        
        rtmp->m_status = ENUM_RTMP_STATUS_HANDSHAKE;
    } else {
        LOG_ERROR("handshake| msg=c2 error|");

        rtmp->m_status = ENUM_RTMP_STATUS_ERROR;
    }
    
    return ret;
}

Int32 RtmpHandler::dealCtrlMsg(Rtmp* rtmp, Cache* cache) {
    Int32 ret = 0;
    Bool bOk = FALSE;
    Uint16 u16 = 0;
    Uint32 val = 0;
    Uint32 p2 = 0;
    Byte ch = 0;
    RtmpPkg* pkg = NULL;
    Parser parser;

    pkg = MsgCenter::cast<RtmpPkg>(cache);
    parser.reset(pkg->m_payload, pkg->m_size);

    switch (pkg->m_rtmp_type) {
    case RTMP_MSG_TYPE_CHUNK_SIZE:
        bOk = parser.parse32(&val);
        if (bOk) {
            LOG_INFO("deal_set_chunk| chunk_size_in=%u| msg=ok|", val);
            
            rtmp->m_chunkSizeIn = val; 
        } else {
            LOG_ERROR("deal_set_chunk| pld_size=%d| msg=parse error|",
                pkg->m_size);
            
            ret = -1;
        }
        
        break;

    case RTMP_MSG_TYPE_BYTES_READ_REPORT:
        bOk = parser.parse32(&val);
        if (bOk) { 
            LOG_INFO("deal_bytes_report| ack_snd_bytes=%u| msg=ok|", val);

            rtmp->m_snd_ack_bytes = val;
        } else {
            LOG_ERROR("deal_bytes_report| pld_size=%d| msg=parse error|",
                pkg->m_size);
            
            ret = -1;
        }
        
        break;

    case RTMP_MSG_TYPE_USR_CTRL_MSG:
        bOk = parser.parse16(&u16);
        if (bOk) {
            bOk = parser.parse32(&val);
            if (bOk) {
                if (0x3 == u16) {
                    bOk = parser.parse32(&p2);
                }
            }
        }

        if (bOk) {
            LOG_INFO("deal_usr_ctl| ev=%u| data=%u| extra=%u| msg=ok|", 
                u16, val, p2);
        } else {
            LOG_ERROR("deal_usr_ctl| ev=%u| data=%u| extra=%u|"
                " msg=parse error|", 
                u16, val, p2);
            ret = -1;
        }
        
        break;

    case RTMP_MSG_TYPE_SERVER_BW:
        bOk = parser.parse32(&val);
        if (bOk) {
            rtmp->m_srv_window_size = val;

            LOG_INFO("deal_serv_bw| rcv_wind_size=%u| msg=ok|", val);
        } else {
            LOG_ERROR("deal_serv_bw| pld_size=%d| msg=parse error|",
                pkg->m_size);
            
            ret = -1;
        }
        
        break;

    case RTMP_MSG_TYPE_CLIENT_BW:
        bOk = parser.parse32(&val);
        if (bOk) {
            bOk = parser.parse8(&ch);
        }
        
        if (bOk) {
            LOG_INFO("deal_client_bw| size=%u| limit=%u| msg=ok|", 
                val, ch);
            
            rtmp->m_cli_window_size = val;
        } else {
            LOG_ERROR("deal_client_bw| pld_size=%d| msg=parse error|",
                pkg->m_size);
            
            ret = -1;
        }

        break;

    default:
        LOG_ERROR("****deal_ctrl_msg| type=%u| pld_size=%d|"
            " msg=type invalid|",
            pkg->m_rtmp_type,
            pkg->m_size);
        
        ret = -1;
        break;
    } 

    return ret;
}

Int32 RtmpHandler::dealRtmp(Rtmp* rtmp, CacheHdr* hdr) {
    Int32 ret = 0;
    Int32 type = 0;

    type = CacheCenter::getType(hdr);
    switch (type) {
    case ENUM_MSG_RTMP_TYPE_AUDIO:
    case ENUM_MSG_RTMP_TYPE_VIDEO:
        ret = dealFlvData(rtmp, hdr);
        break;

    case ENUM_MSG_RTMP_TYPE_INVOKE:
        ret = dealInvoke(rtmp, hdr);
        break;

    case ENUM_MSG_RTMP_TYPE_META_INFO:
        ret = dealMetaData(rtmp, hdr);
        break; 
        
    case ENUM_MSG_RTMP_HANDSHAKE_C0C1:
        ret = dealHandshakeC01(rtmp, hdr);
        break;
    case ENUM_MSG_RTMP_HANDSHAKE_C2:
        ret = dealHandshakeC2(rtmp, hdr);
        break; 
    
    default:
        LOG_INFO("deal_rtmp| rtmp_type=%d| msg=type unknown|", type); 
        break;
    }

    /* release msg */
    CacheCenter::free(hdr);
    return ret;
}

Int32 RtmpHandler::dealInvoke(Rtmp* rtmp, CacheHdr* hdr) {
    Int32 ret = 0;
    Uint32 sid = RTMP_INVALID_SID;
    Uint32 pkg_sid = RTMP_INVALID_SID;
    Bool bOk = FALSE;
    Chunk* method = NULL;
    double* txn = NULL;
    MsgComm* msg = NULL;
    Cache* cache = NULL;
    RtmpPkg* pkg = NULL;
    AMFObject obj = AMF_OBJ_INVALID;

    msg = MsgCenter::body<MsgComm>(hdr);
    sid = msg->m_sid;   // this is the real used sid
    
    cache = CacheCenter::getCache(hdr);
    pkg = MsgCenter::cast<RtmpPkg>(cache);

    /* original sid in the header */
    pkg_sid = pkg->m_sid;
    
    bOk = m_amf->decode(&obj, pkg->m_payload, pkg->m_size);
    if (bOk) {
        m_amf->dump("deal_invoke", &obj);

        method = m_amf->indexPropString(&obj, 0);
        if (NULL != method) {
            txn = m_amf->indexPropNum(&obj, 1);
            
            if (matchAV(&av_connect, method)) {
                ret = dealConn(rtmp, &obj);
            } else if (matchAV(&av_createStream, method)) {
                ret = dealCreatStream(rtmp, &obj);
            } else if (matchAV(&av_getStreamLength, method)) {
                ret = m_proto_center->sendResult<AMF_INVALID, Void>(rtmp, RTMP_INVALID_SID, txn);
            } else if (matchAV(&av_play, method)) {
                ret = dealPlay(rtmp, sid, pkg_sid, &obj);
            } else if (matchAV(&av_publish, method)) {
                ret = dealPublish(rtmp, sid, pkg_sid, &obj);
            } else if (matchAV(&av_pause, method)) {
                ret = dealPause(rtmp, sid, pkg_sid, &obj);
            } else if (matchAV(&av_FCPublish, method)) {
                ret = dealFCPublish(rtmp, sid, pkg_sid, &obj);
            } else if (matchAV(&av_FCUnpublish, method)) {
                ret = dealFCUnpublish(rtmp, sid, pkg_sid, &obj);
            } else if (matchAV(&av_deleteStream, method)) {
                ret = dealDelStream(rtmp, sid, pkg_sid, &obj);
            } else if (matchAV(&av_releaseStream, method)) {
                ret = dealReleaseStream(rtmp, sid, pkg_sid, &obj);
            } else if (matchAV(&av__checkbw, method)) {
                ret = m_proto_center->sendResult<AMF_INVALID, Void>(rtmp, RTMP_INVALID_SID, txn);
            } else {                
                LOG_INFO("deal_invoke| user_id=%u| sid=%u| pkg_sid=%u|"
                    " method=%*.s| msg=ok|",
                    rtmp->m_user_id, sid, pkg_sid,
                    method->m_size, method->m_data);
                    
                ret = 0;
            }
        } else {
            LOG_ERROR("deal_invoke| user_id=%u| sid=%u| pkg_sid=%u|"
                " msg=method null error|",
                rtmp->m_user_id, sid, pkg_sid);

            ret = -1;
        }
    } else {
        LOG_ERROR("deal_invoke| user_id=%u| sid=%u| pkg_sid=%u|"
            " msg=decode error|",
            rtmp->m_user_id, sid, pkg_sid);

        ret = -1;
    }

    m_amf->freeObj(&obj);
    return ret;
}

Int32 RtmpHandler::dealConn(Rtmp* rtmp, AMFObject* obj) {
    Int32 ret = 0;
    double f = 0.;
    double* txn = NULL;
    Chunk* pv = NULL;
    AMFObject* po = NULL;
    AMFObject infoObj = AMF_OBJ_INVALID;
    AMFObject argObj = AMF_OBJ_INVALID;
    AMFObject extObj = AMF_OBJ_INVALID; // this is child of arg

    do { 
        txn = m_amf->indexPropNum(obj, 1); 
        po = m_amf->indexPropObj(obj, 2);
        if (NULL == po) {
            ret = -1;
            break;
        }
    
        /* arguments */
        pv = m_amf->findPropString(po, &av_app);
        if (NULL != pv && 0 < pv->m_size) {
            creatAV(pv, &rtmp->m_app);
        } else {
            ret = -2;
            break;
        } 

        pv = m_amf->findPropString(po, &av_tcUrl);
        if (NULL != pv && 0 < pv->m_size) {
            creatAV(pv, &rtmp->m_tcUrl);
        } else {
            ret = -3;
            break;
        } 

        if (ENUM_RTMP_STATUS_HANDSHAKE != rtmp->m_status) {
            ret = -4;
            break;
        }

        /* generate an unique user id */
        rtmp->m_user_id = ++m_user_id;
            
        /* set bandwidth */
        m_proto_center->sendServBw(rtmp, g_conf.m_window_size);
        m_proto_center->sendCliBw(rtmp, g_conf.m_window_size, 2);

        m_amf->addPropAny<AMF_STRING, Chunk>(&infoObj, &av_fmsVer, &av_def_fmsVer);

        f = 31.0;
        m_amf->addPropAny<AMF_NUMBER, double>(&infoObj, &av_capabilities, &f);

        f = 1.0;
        m_amf->addPropAny<AMF_NUMBER, double>(&infoObj, &av_mode, &f); 

        m_amf->addPropAny<AMF_STRING, Chunk>(&argObj, &av_level, &av_status_level_ok);
        m_amf->addPropAny<AMF_STRING, Chunk>(&argObj, &av_code, &av_status_conn_ok);
        m_amf->addPropAny<AMF_STRING, Chunk>(&argObj, &av_description, &av_status_conn_des);

        f = rtmp->m_encoding;
        m_amf->addPropAny<AMF_NUMBER, double>(&argObj, &av_objectEncoding, &f);

        m_amf->addPropAny<AMF_STRING, Chunk>(&extObj, &av_fms_version, &av_def_fms_version);
        
        m_amf->addPropAny<AMF_OBJECT, AMFObject>(&argObj, &av_fms_data, &extObj); 

        m_proto_center->sendResult<AMF_OBJECT, AMFObject>(rtmp, 
            RTMP_INVALID_SID, txn, &infoObj, &argObj); 
        
        m_proto_center->sendCall<AMF_INVALID, Void>(rtmp, 
            RTMP_INVALID_SID, &av_onBWDone, &AMF_ZERO_DOUBLE); 
        
        rtmp->m_status = ENUM_RTMP_STATUS_CONNECTED; 
    } while (0);

    LOG_INFO("deal_conn| ret=%d| txn=%.2f| user_id=%u|"
        " app=%.*s| tcUrl=%.*s|", 
        ret, 
        F_PTR_VAL(txn),
        rtmp->m_user_id,
        rtmp->m_app.m_size,
        rtmp->m_app.m_data,
        rtmp->m_tcUrl.m_size,
        rtmp->m_tcUrl.m_data);
  
    /* release objs */
    m_amf->freeObj(&infoObj);
    m_amf->freeObj(&argObj);
    return ret;
}

Uint32 RtmpHandler::nextSid(Rtmp* rtmp) {
    Uint32 sid = 0;
    
    for (sid=1; sid<RTMP_MAX_SESS_CNT; ++sid) {
        if (NULL == rtmp->m_unit[sid]) {
            return sid;;
        }
    }

    return RTMP_INVALID_SID;
}

Int32 RtmpHandler::dealCreatStream(Rtmp* rtmp, AMFObject* obj) {
    Int32 ret = 0;
    Uint32 sid = 0;
    double f = 0.;
    double* txn = NULL;

    do {
        txn = m_amf->indexPropNum(obj, 1);

        if (ENUM_RTMP_STATUS_CONNECTED != rtmp->m_status) {
            ret = -1;
            break;
        }

        sid = nextSid(rtmp);
        if (RTMP_INVALID_SID != sid) {
            rtmp->m_unit[sid] = StreamCenter::creatUnit(sid, rtmp);
        } else {
            /* no more sid */
            ret = -2;
            break;
        }
        
        /* set larger chunk */
        m_proto_center->sendChunkSize(rtmp, g_conf.m_chunk_size); 
        
        f = sid;
        m_proto_center->sendResult<AMF_NUMBER, double>(rtmp, 
            RTMP_INVALID_SID, txn, NULL, &f);         

        LOG_INFO("deal_creat_stream| txn=%.2f| user_id=%u|"
            " sid=%u| msg=ok|", 
            F_PTR_VAL(txn), rtmp->m_user_id, sid);
        return 0;
    } while (0);

    LOG_ERROR("deal_creat_stream| ret=%d| txn=%.2f|"
        " user_id=%u| sid=%u| status=%d| msg=error|", 
        F_PTR_VAL(txn), rtmp->m_user_id, sid, rtmp->m_status);
    return ret;
}

Bool RtmpHandler::genStreamKey(const Chunk* app,
    const Chunk* name, Chunk* key) {
    Bool bOk = FALSE;

    bOk = joinAV(app, name, key, DEF_CHUNK_DELIM);
    return bOk;
}

Bool RtmpHandler::isAvcSid(Rtmp* rtmp, Uint32 sid) {
    if (0 < sid && sid < RTMP_MAX_SESS_CNT && NULL != rtmp->m_unit[sid]) {
        return TRUE;
    } else {
        return FALSE;
    }
} 

Int32 RtmpHandler::dealPlay(Rtmp* rtmp, Uint32 sid, 
    Uint32 pkg_sid, AMFObject* obj) {
    Int32 ret = 0;
    Bool bOk = FALSE;
    double* txn = NULL;
    Chunk* pv = NULL;
    RtmpUint* unit = NULL;
    
    do { 
        bOk = isAvcSid(rtmp, sid);
        if (bOk) {
            unit = rtmp->m_unit[sid];
        } else {
            ret = -1;
            break;
        }
        
        txn = m_amf->indexPropNum(obj, 1);

        pv = m_amf->indexPropString(obj, 3); 
        if (NULL != pv && 0 < pv->m_size) {
            creatAV(pv, &unit->m_stream_name);
        } else {
            ret = -1;
            break;
        } 
        
        genStreamKey(&rtmp->m_app, &unit->m_stream_name, &unit->m_key);
        ret = m_stream_center->regPlayer(unit);
        if (0 != ret) {
            break;
        }

        m_proto_center->notifyStart(rtmp, sid, &av_status_play_start,
            RTMP_STREAM_START_DESC); 
        m_stream_center->playBaseAvc(unit);

        LOG_INFO("deal_play| txn=%.2f| user_id=%u| sid=%u|"
            " pkg_sid=%u| app=%.*s| stream_name=%.*s| msg=ok|", 
            F_PTR_VAL(txn), rtmp->m_user_id, sid, pkg_sid, 
            rtmp->m_app.m_size,
            rtmp->m_app.m_data,
            unit->m_stream_name.m_size,
            unit->m_stream_name.m_data);

        return 0;
    } while (0);

    LOG_ERROR("deal_play| ret=%d| txn=%.2f| user_id=%u| sid=%u|"
        " pkg_sid=%u| app=%.*s| stream_name=%.*s| msg=error|", 
        ret, F_PTR_VAL(txn), rtmp->m_user_id, sid, pkg_sid,
        rtmp->m_app.m_size,
        rtmp->m_app.m_data,
        unit->m_stream_name.m_size,
        unit->m_stream_name.m_data);
    
    return ret;
}

Int32 RtmpHandler::dealPause(Rtmp* rtmp, Uint32 sid, 
    Uint32 pkg_sid, AMFObject* obj) {
    Int32 ret = 0;
    Bool bOk = FALSE;
    double* txn = NULL;
    Int32* pi = NULL;
    RtmpUint* unit = NULL;

    do { 
        bOk = isAvcSid(rtmp, sid);
        if (bOk) {
            unit = rtmp->m_unit[sid];
        } else {
            ret = -1;
            break;
        }
        
        txn = m_amf->indexPropNum(obj, 1);
        
        pi = m_amf->indexPropBool(obj, 3);
        if (NULL != pi) {
            unit->m_is_pause = *pi;
        } else {
            ret = -1;
            break;
        }

        if (unit->m_is_pause) {    
            m_proto_center->notifyStop(rtmp, sid, 
                &av_netstream_pause_notify, RTMP_STREAM_PAUSE_DESC);  
        } else {
            m_proto_center->notifyStart(rtmp, sid, 
                &av_netstream_unpause_notify, RTMP_STREAM_UNPAUSE_DESC); 
        }
    } while (0);

    LOG_INFO("deal_pause| ret=%d| txn=%.2f| user_id=%u| sid=%u|"
        " pkg_sid=%u| app=%.*s| stream_name=%.*s|", 
        ret, F_PTR_VAL(txn), rtmp->m_user_id, sid, pkg_sid,
        rtmp->m_app.m_size,
        rtmp->m_app.m_data,
        unit->m_stream_name.m_size,
        unit->m_stream_name.m_data);

    return ret;
}

Int32 RtmpHandler::dealPublish(Rtmp* rtmp, Uint32 sid, 
    Uint32 pkg_sid, AMFObject* obj) {
    Int32 ret = 0;
    Bool bOk = FALSE;
    double* txn = NULL;
    Chunk* pv = NULL;
    RtmpUint* unit = NULL;

    do { 
        bOk = isAvcSid(rtmp, sid);
        if (bOk) {
            unit = rtmp->m_unit[sid];
        } else {
            ret = -1;
            break;
        }
        
        txn = m_amf->indexPropNum(obj, 1);

        pv = m_amf->indexPropString(obj, 3); 
        if (NULL != pv && 0 < pv->m_size) {
            creatAV(pv, &unit->m_stream_name);
        } else {
            ret = -1;
            break;
        } 

        pv = m_amf->indexPropString(obj, 4);
        if (NULL != pv) {
            creatAV(pv, &unit->m_stream_type);
        } else {
            ret = -1;
            break;
        }
        
        genStreamKey(&rtmp->m_app, &unit->m_stream_name, &unit->m_key);
        ret = m_stream_center->regPublisher(unit);
        if (0 != ret) {
            break;
        }

        m_proto_center->notifyStart(rtmp, sid, &av_netstream_publish_start,
            RTMP_STREAM_PUBLISH_DESC); 

        LOG_INFO("deal_publish| txn=%.2f| user_id=%u| sid=%u|"
            " pkg_sid=%u| app=%.*s| stream_name=%.*s| msg=ok|", 
            F_PTR_VAL(txn), rtmp->m_user_id, sid, pkg_sid,
            rtmp->m_app.m_size,
            rtmp->m_app.m_data,
            unit->m_stream_name.m_size,
            unit->m_stream_name.m_data);

        return 0;
    } while (0);

    LOG_ERROR("deal_publish| ret=%d| txn=%.2f| user_id=%u| sid=%u|"
        " pkg_sid=%u| app=%.*s| stream_name=%.*s| msg=error|", 
        ret, F_PTR_VAL(txn), rtmp->m_user_id, sid, pkg_sid,
        rtmp->m_app.m_size,
        rtmp->m_app.m_data,
        unit->m_stream_name.m_size,
        unit->m_stream_name.m_data);
    
    return ret;
}

RtmpStream* RtmpHandler::findStream(Rtmp* rtmp, const Chunk* name) {
    RtmpStream* stream = NULL;
    Chunk chunk = AMF_CHUNK_EMPTY;
    typeString strKey;

    genStreamKey(&rtmp->m_app, name, &chunk);
    StreamCenter::toString(&chunk, strKey);

    stream = m_stream_center->findStream(strKey);

    freeAV(&chunk);
    return stream;
}

Int32 RtmpHandler::dealFCPublish(Rtmp* rtmp, Uint32 sid, 
    Uint32 pkg_sid, AMFObject* obj) {
    Int32 ret = 0;
    double* txn = NULL;
    Chunk* path = NULL;
    RtmpStream* stream = NULL;

    do {
        txn = m_amf->indexPropNum(obj, 1);
        
        path = m_amf->indexPropString(obj, 3); 
        if (NULL == path) {
            ret = -1;
            break;
        } 

        stream = findStream(rtmp, path);
        if (NULL == stream) {
            ret = 0;
            break;
        }

        m_proto_center->sendCall<AMF_INVALID, Void>(rtmp, sid, &av_onFCPublish, &AMF_ZERO_DOUBLE);
        m_stream_center->publishFC(stream, path);  
    } while (0);

    LOG_INFO("deal_FCPublish| ret=%d| txn=%.2f|"
            " user_id=%u| sid=%u| pkg_sid=%u| path=%.*s|", 
            ret, F_PTR_VAL(txn), rtmp->m_user_id,
            sid, pkg_sid,
            path->m_size, path->m_data);

    return ret;
}

Int32 RtmpHandler::dealFCUnpublish(Rtmp* rtmp, Uint32 sid, 
    Uint32 pkg_sid, AMFObject* obj) {
    Int32 ret = 0;
    double* txn = NULL;
    Chunk* path = NULL;
    RtmpStream* stream = NULL;

    do {
        txn = m_amf->indexPropNum(obj, 1);
        
        path = m_amf->indexPropString(obj, 3); 
        if (NULL == path) {
            ret = -1;
            break;
        } 

        stream = findStream(rtmp, path);
        if (NULL == stream) {
            ret = 0;
            break;
        }

        m_stream_center->unpublishFC(stream, path);  
    } while (0);

    LOG_INFO("deal_FCUnpublish| ret=%d| txn=%.2f|"
            " user_id=%u| sid=%u| pkg_sid=%u| path=%.*s|", 
            ret, F_PTR_VAL(txn), rtmp->m_user_id,
            sid, pkg_sid,
            path->m_size, path->m_data);

    return ret;
}

Int32 RtmpHandler::dealReleaseStream(Rtmp* rtmp, Uint32 sid, 
    Uint32 pkg_sid, AMFObject* obj) {
    Int32 ret = 0;
    double* txn = NULL;
    Chunk* path = NULL;
    Chunk chunk = AMF_CHUNK_EMPTY;
    RtmpStream* stream = NULL;

    do {
        txn = m_amf->indexPropNum(obj, 1);
        
        path = m_amf->indexPropString(obj, 3); 
        if (NULL == path) {
            ret = -1;
            break;
        } 

        stream = findStream(rtmp, path);
        if (NULL == stream) {
            ret = 0;
            break;
        }

        m_stream_center->stopPublisher(stream);   
    } while (0);

    LOG_INFO("deal_release_stream| ret=%d| txn=%.2f| user_id=%u|"
        " sid=%u| pkg_sid=%u| stream_name=%.*s|", 
        ret, F_PTR_VAL(txn), rtmp->m_user_id,
        sid, pkg_sid,
        path->m_size, path->m_data); 
    
    freeAV(&chunk);
    return ret;
}

Int32 RtmpHandler::dealDelStream(Rtmp* rtmp, Uint32 sid, 
    Uint32 pkg_sid, AMFObject* obj) {
    Int32 ret = 0;
    Bool bOk = TRUE;
    Uint32 req_sid = RTMP_INVALID_SID;
    double* txn = NULL;
    double* pf = NULL;

    do {
        txn = m_amf->indexPropNum(obj, 1);
        
        pf = m_amf->indexPropNum(obj, 3); 
        if (NULL != pf) {
            req_sid = (Uint32)*pf;
        } else {
            ret = -1;
            break;
        } 

        bOk = isAvcSid(rtmp, req_sid);
        if (bOk) {
            m_stream_center->stopUnit(rtmp, req_sid, FALSE);
        } else {
            break;
        } 
    } while (0);
    
    LOG_INFO("deal_deleteStream| ret=%d| txn=%.2f|"
        " user_id=%u| req_sid=%u| sid=%u| pkg_sid=%u|", 
        ret, F_PTR_VAL(txn), rtmp->m_user_id, req_sid,
        sid, pkg_sid);

    return ret;
}

Int32 RtmpHandler::dealMetaData(Rtmp* rtmp, CacheHdr* hdr) {
    Int32 ret = 0;
    Int32 cnt = 0;
    Bool bOk = FALSE;
    Cache* cache = NULL;
    RtmpPkg* pkg = NULL;
    Chunk chunk = AMF_CHUNK_EMPTY;
    AMFObject obj = AMF_OBJ_INVALID;

    cache = CacheCenter::getCache(hdr);
    pkg = MsgCenter::cast<RtmpPkg>(cache); 

    do {        
        if (!pkg->m_has_striped) {
            pkg->m_has_striped = TRUE;
            
            cnt = m_amf->peekString(pkg->m_payload, pkg->m_size, &chunk);
            if (0 >= cnt) {
                LOG_ERROR("deal_meta_data| msg=decode string error|");

                ret = -1;
                break;
            }

            // strip '@metadata'
            if (matchAV(&av_meta_prefix_data, &chunk)) {
                pkg->m_skip = cnt;

                LOG_INFO("deal_meta_data| skip=%d|", pkg->m_skip);
            } 

            bOk = m_amf->decode(&obj, pkg->m_payload + pkg->m_skip, 
                pkg->m_size - pkg->m_skip);
            if (!bOk) { 
                ret = -2;
                break;
            }

            m_amf->dump("deal_meta_data", &obj); 
        }

        /* go to flv data handler */
        ret = dealFlvData(rtmp, hdr);
        return ret;
    } while (0);
    
    LOG_ERROR("deal_meta_data| ret=%d| user_id=%u| pkg_sid=%u| msg=error|",
        ret, rtmp->m_user_id, pkg->m_sid);
    
    m_amf->freeObj(&obj);
    return ret;
}

Int32 RtmpHandler::publish(RtmpUint* unit, Cache* cache) {
    Int32 ret = 0;
    Int32 msg_type = 0;
    Int32 payload_size = 0;
    Uint32 pkg_sid = 0;
    Byte* data = NULL;
    RtmpPkg* pkg = NULL;

    pkg = MsgCenter::cast<RtmpPkg>(cache);
    msg_type = (Int32)pkg->m_msg_type;
    pkg_sid = pkg->m_sid;
    data = pkg->m_payload + pkg->m_skip;
    payload_size = pkg->m_size - pkg->m_skip;
    
    /* publisher cache some data */
    switch (msg_type) {
    case ENUM_MSG_RTMP_TYPE_AUDIO:
        if (2 <= payload_size && m_stream_center->chkAudioSeq(data)) {
            m_stream_center->cacheAvc(unit, ENUM_AVC_AUDIO, cache);
        } 
        
        break;

    case ENUM_MSG_RTMP_TYPE_VIDEO:
        if (2 <= payload_size && m_stream_center->chkVideoSeq(data)) {
            m_stream_center->cacheAvc(unit, ENUM_AVC_VEDIO, cache);
        }
        
        break;

    case ENUM_MSG_RTMP_TYPE_META_INFO:
        m_stream_center->cacheAvc(unit, ENUM_AVC_META_DATA, cache);
        break;

    default:
        break;
    }

    if (!unit->m_blocked) {
        m_stream_center->publish(unit->m_ctx, msg_type, cache); 
    }

    LOG_DEBUG("deal_flv_publish| ret=%d| msg_type=%d|"
        " user_id=%u| sid=%u| pkg_sid=%u| blocked=%d| paused=%d|", 
        ret, msg_type, 
        unit->m_parent->m_user_id,
        unit->m_sid, pkg_sid,
        unit->m_blocked,
        unit->m_is_pause);

    return ret;
}

Int32 RtmpHandler::play(RtmpUint* unit, Cache* cache) {
    Int32 ret = 0;
    Int32 msg_type = 0;
    Uint32 pkg_sid = 0;
    Byte* data = NULL;
    RtmpPkg* pkg = NULL;

    pkg = MsgCenter::cast<RtmpPkg>(cache);
    msg_type = (Int32)pkg->m_msg_type;
    pkg_sid = pkg->m_sid;
    data = pkg->m_payload + pkg->m_skip;

    /* players, and is not blocked */
    if (!unit->m_blocked) {
        if (!unit->m_is_pause) {
            /* normal status to play, */
            ret = m_stream_center->sendFlv(unit, cache);
        } else {
            /* pause status, try to find key frame, then block */
            if (ENUM_MSG_RTMP_TYPE_VIDEO == pkg->m_msg_type
                && m_stream_center->chkVideoKeyFrame(data)) {
                unit->m_blocked = TRUE;
            } else {
                ret = m_stream_center->sendFlv(unit, cache);
            }
        }
    } else {
        if (unit->m_is_pause) {
            /* be in blocked status, and pressed pause, ignore pkg */
        } else {
            /* try to find the first key frame, then unblock msgs */
            if (ENUM_MSG_RTMP_TYPE_VIDEO == pkg->m_msg_type
                && m_stream_center->chkVideoKeyFrame(data)) {
                unit->m_blocked = FALSE;

                ret = m_stream_center->sendFlv(unit, cache);
            }
        }
    }

    LOG_DEBUG("deal_flv_play| ret=%d| msg_type=%d|"
        " user_id=%u| sid=%u| pkg_sid=%u| blocked=%d| paused=%d|", 
        ret, msg_type, 
        unit->m_parent->m_user_id,
        unit->m_sid, pkg_sid,
        unit->m_blocked,
        unit->m_is_pause);

    return ret;
}

Int32 RtmpHandler::dealFlvData(Rtmp* rtmp, CacheHdr* hdr) {
    Int32 ret = 0;
    Bool bOk = FALSE;
    Int32 msg_type = 0;
    Uint32 sid = 0;
    Uint32 pkg_sid = 0;
    MsgComm* msg = NULL;
    Cache* cache = NULL;
    RtmpPkg* pkg = NULL;
    RtmpUint* unit = NULL;

    msg = MsgCenter::body<MsgComm>(hdr);
    sid = msg->m_sid;   // this is the real used sid
    
    cache = CacheCenter::getCache(hdr); 
    pkg = MsgCenter::cast<RtmpPkg>(cache);
    msg_type = (Int32)pkg->m_msg_type;
    pkg_sid = pkg->m_sid;   // this is the original pkg sid

    do {
        bOk = isAvcSid(rtmp, sid);
        if (bOk) {
            unit = rtmp->m_unit[sid];
        } else {
            LOG_ERROR("deal_flv_data| msg_type=%d|"
                " user_id=%u| sid=%u| pkg_sid=%u|"
                " msg=invalid sid|", 
                msg_type, rtmp->m_user_id, sid, pkg_sid);
            
            ret = -1;
            break;
        }
        
        if (ENUM_OPER_PLAYER == unit->m_oper_auth) {
            ret = play(unit, cache); 
        } else if (ENUM_OPER_PUBLISHER == unit->m_oper_auth) {
            ret = publish(unit, cache);
        } else {
            LOG_ERROR("deal_flv_data| msg_type=%d|"
                " user_id=%u| sid=%u| pkg_sid=%u| oper_auth=%d|"
                " msg=invalid oper_auth|", 
                msg_type, rtmp->m_user_id, 
                sid, pkg_sid, unit->m_oper_auth);

            ret = -1;
        }
    } while (0);
     
    return ret;
}

Int32 RtmpHandler::sendBytesReport(Rtmp* rtmp, Uint32 seqBytes) {
    Int32 ret = 0;

    ret = m_proto_center->sendBytesReport(rtmp, seqBytes);
    return ret;
}

Void RtmpHandler::closeRtmp(Rtmp* rtmp) {
    rtmp->m_status = ENUM_RTMP_STATUS_CLOSED;
    
    for (Uint32 i=1; i<RTMP_MAX_SESS_CNT; ++i) { 
        if (NULL != rtmp->m_unit[i]) {
            m_stream_center->stopUnit(rtmp, i, FALSE);
        }
    }
}

