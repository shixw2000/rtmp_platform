#include"invokepld.h"
#include"cache.h"
#include"rtmpnode.h"
#include"analyser.h"
#include"payload.h"
#include"handshake.h"
#include"msgtype.h"
#include"streamcenter.h"


RtmpHandler::RtmpHandler() {
    m_handshake = NULL;
    m_amf = NULL;
    m_stream_center = NULL;
}

RtmpHandler::~RtmpHandler() {
}

Int32 RtmpHandler::init() {
    I_NEW(HandShake, m_handshake);
    I_NEW(AmfPayload, m_amf);
    I_NEW(StreamCenter, m_stream_center);

    return 0;
}

Void RtmpHandler::finish() {
    I_FREE(m_handshake);
    I_FREE(m_amf);
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

Int32 RtmpHandler::sendCmd(RtmpNode* node, Uint32 stream_id,
    Uint32 rtmp_type, const Chunk* chunk) {
    Int32 ret = 0;

    ret = node->sendPayload(node, 0, stream_id, rtmp_type, chunk);
    return ret;
}

Int32 RtmpHandler::sendChunkSize(RtmpNode* node, Rtmp* rtmp,
    Uint32 chunkSize) {
    Int32 ret = 0;
    Byte buff[RTMP_MIN_CHUNK_SIZE] = {0};
    Chunk chunk = {buff, RTMP_MIN_CHUNK_SIZE};
    Builder builder(&chunk); 
    
    builder.build32(&chunkSize);
    chunk.m_size = builder.used();
    chunk.m_data = buff;

    ret = sendCmd(node, 0, RTMP_MSG_TYPE_CHUNK_SIZE, &chunk);
    if (0 == ret) { 
        LOG_INFO("send_chunk_size| chunk_size_out=%u|"
            " msg=send amf ok|", chunkSize);
        
        /* update out chunk size */
        rtmp->m_chunkSizeOut = chunkSize;
    } else {
        LOG_ERROR("send_chunk_size| chunk_size_out=%u|"
            " msg=send amf error|", chunkSize);
    }

    return ret;
}

Int32 RtmpHandler::sendBytesReport(RtmpNode* node, Rtmp*,
    Uint32 seqBytes) {
    Int32 ret = 0;
    Byte buff[RTMP_MIN_CHUNK_SIZE] = {0};
    Chunk chunk = {buff, RTMP_MIN_CHUNK_SIZE};
    Builder builder(&chunk); 

    builder.build32(&seqBytes);

    chunk.m_size = builder.used();
    chunk.m_data = buff;
    
    ret = sendCmd(node, 0, RTMP_MSG_TYPE_BYTES_READ_REPORT, &chunk);
    if (0 == ret) { 
        LOG_INFO("send_bytes_report| ack_rcv_bytes=%u| msg=send amf ok|", 
            seqBytes); 
    } else {
        LOG_ERROR("send_bytes_report| ack_rcv_bytes=%u| msg=send amf error|", 
            seqBytes);
    }
  
    return ret;
}

Int32 RtmpHandler::sendServBw(RtmpNode* node, Rtmp* rtmp, Uint32 bw) {
    Int32 ret = 0;
    Byte buff[RTMP_MIN_CHUNK_SIZE] = {0};
    Chunk chunk = {buff, RTMP_MIN_CHUNK_SIZE};
    Builder builder(&chunk); 

    builder.build32(&bw);

    chunk.m_size = builder.used();
    chunk.m_data = buff; 

    ret = sendCmd(node, 0, RTMP_MSG_TYPE_SERVER_BW, &chunk);
    if (0 == ret) { 
        LOG_INFO("send_serv_bw| snd_window_size=%u| msg=send amf ok|", bw);
        
        /* update send window size */
        rtmp->m_srv_window_size = bw;
    } else {
        LOG_ERROR("send_serv_bw| snd_window_size=%u| msg=send amf error|", bw);
    }

    return ret;
}

Int32 RtmpHandler::sendCliBw(RtmpNode* node, Rtmp* rtmp,
    Uint32 bw, Int32 limit){
    Int32 ret = 0;
    Byte ch = (Byte)limit;
    Byte buff[RTMP_MIN_CHUNK_SIZE] = {0};
    Chunk chunk = {buff, RTMP_MIN_CHUNK_SIZE};
    Builder builder(&chunk); 

    builder.build32(&bw);
    builder.build8(&ch);

    chunk.m_size = builder.used();
    chunk.m_data = buff; 

    ret = sendCmd(node, 0, RTMP_MSG_TYPE_CLIENT_BW, &chunk); 
    if (0 == ret) { 
        LOG_INFO("send_cli_bw| size=%u| limit=%u| msg=send amf ok|", 
            bw, ch);

        rtmp->m_cli_window_size = bw;
    } else {
        LOG_ERROR("send_cli_bw| size=%u| limit=%u| msg=send amf error|",
            bw, ch);
    }
    
    return ret;
}

Int32 RtmpHandler::sendUsrCtrl(RtmpNode* node, Uint16 ev, 
    Uint32 p1, Uint32 p2) {
    Int32 ret = 0;
    Byte buff[RTMP_MIN_CHUNK_SIZE] = {0};
    Chunk chunk = {buff, RTMP_MIN_CHUNK_SIZE};
    Builder builder(&chunk); 

    if (0x3 == ev) { 
        builder.build16(&ev);
        builder.build32(&p1);
        builder.build32(&p2);
    } else { 
        builder.build16(&ev);
        builder.build32(&p1);
    }

    chunk.m_size = builder.used();
    chunk.m_data = buff; 

    ret = sendCmd(node, 0, RTMP_MSG_TYPE_USR_CTRL_MSG, &chunk); 
    if (0 == ret) { 
        LOG_INFO("send_usr_ctrl| ev=%u| data=%u| extra=%u| msg=send amf ok|", 
            ev, p1, p2);
    } else {
        LOG_ERROR("send_usr_ctrl| ev=%u| data=%u| extra=%u| msg=send amf error|",
            ev, p1, p2);
    }
    
    return ret;
}

Int32 RtmpHandler::sendRtmpPkg(RtmpNode* node,
    Uint32 stream_id, Cache* cache) {
    Int32 ret = 0;

    ret = node->sendPkg(node, stream_id, cache); 
    return ret;
}

Int32 RtmpHandler::sendAmfObj(const Char* promt, RtmpNode* node, 
    Uint32 epoch, Uint32 stream_id,
    Uint32 rtmp_type, AMFObject* obj) {
    Int32 ret = 0;
    Bool bOk = FALSE;
    Chunk chunk = AMF_CHUNK_EMPTY;

    m_amf->dump(promt, obj);
    
    bOk = m_amf->encode(obj, &chunk);
    if (bOk) {
        ret = node->sendPayload(node, epoch, stream_id, rtmp_type, &chunk); 
    } else {
        ret = -1;
    } 

    return ret;
}

Int32 RtmpHandler::sendStatus(RtmpNode* node, const Chunk* status, 
    const Chunk* detail) {
    Int32 ret = 0;
    AMFObject arg = AMF_OBJ_INVALID;

    m_amf->addPropAny(&arg, &av_level, AMF_STRING, &av_status_level_ok);
    m_amf->addPropAny(&arg, &av_code, AMF_STRING, status);
    m_amf->addPropAny(&arg, &av_description, AMF_STRING, &av_status_ok_des);
    m_amf->addPropAny(&arg, &av_details, AMF_STRING, detail);
    m_amf->addPropAny(&arg, &av_clientid, AMF_STRING, &av_def_clientid);
  
    ret = sendCall(node, &av_onStatus, &AMF_ZERO_DOUBLE, 
        NULL, AMF_OBJECT, &arg); 
    return ret;
}

Int32 RtmpHandler::sendCall(RtmpNode* node, const Chunk* call, 
    const double* txn, AMFObject* info, 
    AMFDataType arg_type, const Void* arg) {
    Int32 ret = 0;
    AMFObject obj = AMF_OBJ_INVALID;

    m_amf->addPropAny(&obj, NULL, AMF_STRING, call);
    m_amf->addPropAny(&obj, NULL, AMF_NUMBER, txn);

    if (NULL != info) {
        m_amf->addPropAny(&obj, NULL, AMF_OBJECT, info);
    } else {
        m_amf->addPropAny(&obj, NULL, AMF_NULL, NULL); 
    }

    if (AMF_INVALID != arg_type) {
        m_amf->addPropAny(&obj, NULL, arg_type, arg); 
    } 
    
    ret = sendAmfObj("send_call", node, 0, 0, RTMP_MSG_TYPE_INVOKE, &obj);
    if (0 == ret) {
        LOG_DEBUG("send_call| call=%.*s| txn=%.2f|"
            " has_info=%d| arg_type=0x%x|"
            " msg=send amf ok|",
            call->m_size, call->m_data, *txn,
            !!info, arg_type);
    } else {
        LOG_ERROR("send_call| call=%.*s| txn=%.2f|"
            " has_info=%d| arg_type=0x%x|"
            " msg=send amf error|",
            call->m_size, call->m_data, *txn,
            !!info, arg_type);
    }

    m_amf->resetObj(&obj); 
    return ret;
}


Int32 RtmpHandler::dealHandshakeC01(RtmpNode* node, Rtmp* rtmp,
    CacheHdr* hdr) {
    Int32 ret = 0;
    Cache* cache = NULL;
    CommPkg* pkg = NULL;
    RtmpHandshake* rtmpHandshake = &rtmp->m_chn.m_handshake;

    cache = CacheCenter::getCache(hdr);
    pkg = MsgCenter::cast<CommPkg>(cache);
    
    ret = m_handshake->parseC0C1(pkg->m_data, rtmpHandshake);
    if (0 == ret) {
        /* reused the cache to create s0s1 */        
        m_handshake->creatS0S1(pkg->m_data, rtmpHandshake);

        /* send c01 and this cache is free by hdr in function end */
        node->handshake(node, cache);
        
        /* creat s2 */ 
        cache = MsgCenter::creatCache<CommPkg>(RTMP_SIG_SIZE);
        pkg = MsgCenter::cast<CommPkg>(cache);
        pkg->m_size = RTMP_SIG_SIZE;
        
        m_handshake->creatS2(pkg->m_data, rtmpHandshake);

        /* send s2 */
        node->handshake(node, cache);
        
        /* release cache of s2 here */
        CacheCenter::del(cache);
        
        LOG_INFO("handshake| msg=c01 ok|");
    } else {
        LOG_ERROR("handshake| msg=c01 error|");

        rtmp->m_status = ENUM_RTMP_STATUS_ERROR;
    }

    return ret;
}

Int32 RtmpHandler::dealHandshakeC2(RtmpNode*, Rtmp* rtmp,
    CacheHdr* hdr) {
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

Int32 RtmpHandler::dealCtrlMsg(RtmpNode*, Rtmp* rtmp, Cache* cache) {
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

Int32 RtmpHandler::dealRtmp(RtmpNode* node, Rtmp* rtmp, CacheHdr* hdr) {
    Int32 ret = 0;
    Int32 type = 0;

    type = CacheCenter::getType(hdr);
    switch (type) {
    case ENUM_MSG_RTMP_TYPE_AUDIO:
    case ENUM_MSG_RTMP_TYPE_VIDEO:
        ret = dealFlvData(node, rtmp, type, hdr);
        break;

    case ENUM_MSG_RTMP_TYPE_INVOKE:
        ret = dealInvoke(node, rtmp, hdr);
        break;

    case ENUM_MSG_RTMP_TYPE_META_INFO:
        ret = dealMetaData(node, rtmp, hdr);
        break; 
        
    case ENUM_MSG_RTMP_HANDSHAKE_C0C1:
        ret = dealHandshakeC01(node, rtmp, hdr);
        break;
    case ENUM_MSG_RTMP_HANDSHAKE_C2:
        ret = dealHandshakeC2(node, rtmp, hdr);
        break; 

    case ENUM_MSG_CUSTOMER_NOTIFY_END_STREAM:
        ret = dealNotifyEndStream(node, rtmp, hdr);
        break;
    
    default:
        LOG_INFO("deal_rtmp| rtmp_type=%d| msg=type unknown|", type); 
        break;
    }

    /* release msg */
    CacheCenter::free(hdr);
    return ret;
}

Int32 RtmpHandler::dealInvoke(RtmpNode* node, Rtmp* rtmp, CacheHdr* hdr) {
    Int32 ret = 0;
    Bool bOk = FALSE;
    AMFObject obj = AMF_OBJ_INVALID;
    Chunk* method = NULL;
    double* txn = NULL;
    double f = 0.;
    Cache* cache = NULL;
    RtmpPkg* pkg = NULL;
    
    cache = CacheCenter::getCache(hdr);
    pkg = MsgCenter::cast<RtmpPkg>(cache);

    bOk = m_amf->decode(&obj, pkg->m_payload, pkg->m_size);
    if (bOk) {
        m_amf->dump("deal_invoke", &obj);

        method = m_amf->indexPropString(&obj, 0);
        if (NULL != method) {
            txn = m_amf->indexPropNum(&obj, 1);
            
            if (matchAV(&av_connect, method)) {
                ret = dealConn(node, rtmp, &obj);
            } else if (matchAV(&av_createStream, method)) {
                ret = dealCreatStream(node, rtmp, &obj);
            } else if (matchAV(&av_getStreamLength, method)) {
                f = 10.;
                ret = sendCall(node, &av__result, txn, NULL, AMF_NUMBER, &f);
            } else if (matchAV(&av_play, method)) {
                ret = dealPlay(node, rtmp, &obj);
            } else if (matchAV(&av_FCPublish, method)) {                
                ret = sendCall(node, &av_onFCPublish, &AMF_ZERO_DOUBLE,
                    NULL, AMF_INVALID, NULL);
            } else if (matchAV(&av_publish, method)) {
                ret = dealPublish(node, rtmp, &obj);
            } else if (matchAV(&av_pause, method)) {
                ret = dealPause(node, rtmp, &obj);
            } else if (matchAV(&av_deleteStream, method)) {
                ret = dealDelStream(node, rtmp, &obj);
            } else if (matchAV(&av_FCUnpublish, method)) {
                ret = dealUnpublish(node, rtmp, &obj);
            } else if (matchAV(&av_closeStream, method)) {
                ret = 0;
            } else if (matchAV(&av_releaseStream, method)
                || matchAV(&av__checkbw, method)) {
                ret = sendCall(node, &av__result, txn, NULL, AMF_INVALID, NULL);
            } else {
                ret = 0;
            }
        } else {
            LOG_ERROR("deal_invoke| msg=method null error|");

            ret = -1;
        }
    } else {
        LOG_ERROR("deal_invoke| msg=decode error|");

        ret = -1;
    }

    m_amf->resetObj(&obj);
    return ret;
}

Int32 RtmpHandler::dealConn(RtmpNode* node, Rtmp* rtmp, AMFObject* obj) {
    Int32 ret = 0;
    double f = 0.;
    double* txn = NULL;
    Chunk* pv = NULL;
    AMFObject* po = NULL;
    AMFObject infoObj = AMF_OBJ_INVALID;
    AMFObject argObj = AMF_OBJ_INVALID;
    AMFObject extObj = AMF_OBJ_INVALID;

    do {
        rtmp->m_unit = creatUnit(m_stream_center->genUserId(), node);
        if (NULL == rtmp->m_unit) {
            ret = -1;
            break;
        }

        txn = m_amf->indexPropNum(obj, 1); 
        po = m_amf->indexPropObj(obj, 2);
        if (NULL == po) {
            ret = -1;
            break;
        }
    
        /* arguments */
        pv = m_amf->findPropString(po, &av_app);
        if (NULL != pv) {
            creatAV(pv, &rtmp->m_unit->m_app);
        } else {
            ret = -1;
            break;
        } 

        pv = m_amf->findPropString(po, &av_tcUrl);
        if (NULL != pv) {
            creatAV(pv, &rtmp->m_unit->m_tcUrl);
        } else {
            ret = -1;
            break;
        } 
            
        /* set bandwidth */
        sendServBw(node, rtmp, RTMP_DEF_WIND_SIZE);
        sendCliBw(node, rtmp, RTMP_DEF_WIND_SIZE, 2);

        m_amf->addPropAny(&infoObj, &av_fmsVer, AMF_STRING, &av_def_fmsVer);

        f = 31.0;
        m_amf->addPropAny(&infoObj, &av_capabilities, AMF_NUMBER, &f);

        f = 1.0;
        m_amf->addPropAny(&infoObj, &av_mode, AMF_NUMBER, &f); 

        m_amf->addPropAny(&argObj, &av_level, AMF_STRING, &av_status_level_ok);
        m_amf->addPropAny(&argObj, &av_code, AMF_STRING, &av_status_conn_ok);
        m_amf->addPropAny(&argObj, &av_description, AMF_STRING, &av_status_conn_des);

        f = rtmp->m_unit->m_encoding;
        m_amf->addPropAny(&argObj, &av_objectEncoding, AMF_NUMBER, &f);

        m_amf->addPropAny(&extObj, &av_fms_version, AMF_STRING, &av_def_fms_version);
        m_amf->addPropAny(&argObj, &av_fms_data, AMF_OBJECT, &extObj); 

        ret = sendCall(node, &av__result, txn, &infoObj, AMF_OBJECT, &argObj); 
        if (0 != ret) {
            LOG_ERROR("****deal_conn| txn=%.2f| msg=send conn error|", *txn);
            break;
        } 

        /* send call */
        ret = sendCall(node, &av_onBWDone, &AMF_ZERO_DOUBLE, NULL, AMF_INVALID, NULL);
        if (0 != ret) {
            LOG_ERROR("****deal_conn| txn=%.2f| msg=send call error|", *txn);
            break;
        } 

        rtmp->m_status = ENUM_RTMP_STATUS_CONNECTED;

        LOG_INFO("deal_conn| txn=%.2f| msg=ok|", *txn);
    } while (0);
  
    return ret;
}

Int32 RtmpHandler::dealCreatStream(RtmpNode* node, 
    Rtmp* rtmp, AMFObject* obj) {
    Int32 ret = 0;
    double f = 0.;
    double* txn = NULL;

    do {
        txn = m_amf->indexPropNum(obj, 1);

        /* creat stream id */
        f = ++rtmp->m_unit->m_stream_id;
        
        /* set larger chunk */
        sendChunkSize(node, rtmp, RTMP_DEF_CHUNK_SIZE); 
        
        ret = sendCall(node, &av__result, txn, NULL, AMF_NUMBER, &f); 
        if (0 != ret) {
            break;
        } 
        
        rtmp->m_status = ENUM_RTMP_STATUS_CREATED_STREAM;

        LOG_INFO("deal_creat_stream| txn=%.2f| stream_id=%u| msg=ok|", 
            *txn, rtmp->m_unit->m_stream_id);
    } while (0);
    
    return ret;
}

Bool RtmpHandler::genStreamKey(const Chunk* app,
    const Chunk* name, Chunk* key) {
    Bool bOk = FALSE;

    bOk = joinAV(app, name, key, DEF_CHUNK_DELIM);
    return bOk;
}

Bool RtmpHandler::chkFlvData(Uint32 msg_type) {
    if (ENUM_MSG_RTMP_TYPE_AUDIO == msg_type
        || ENUM_MSG_RTMP_TYPE_VIDEO == msg_type
        || ENUM_MSG_RTMP_TYPE_META_INFO == msg_type) {
        return TRUE;
    } else {
        return FALSE;
    }
}
    
Int32 RtmpHandler::dealPlay(RtmpNode* node, Rtmp* rtmp, AMFObject* obj) {
    Int32 ret = 0;
    Bool bOk = FALSE;
    double* txn = NULL;
    Chunk* pv = NULL;
    
    do {
        txn = m_amf->indexPropNum(obj, 1);

        pv = m_amf->indexPropString(obj, 3); 
        if (NULL != pv) {
            creatAV(pv, &rtmp->m_unit->m_stream_name);
        } else {
            ret = -1;
            break;
        } 
        
        genStreamKey(&rtmp->m_unit->m_app, &rtmp->m_unit->m_stream_name,
            &rtmp->m_unit->m_key);

        rtmp->m_unit->m_is_publisher = FALSE;

        bOk = m_stream_center->regPlayer(rtmp->m_unit);
        if (!bOk) { 
            ret = -1;
            break;
        }

        rtmp->m_status = ENUM_RTMP_STATUS_AUTH_OPER; 
  
        /* user control begin stream */
        sendUsrCtrl(node, 0, rtmp->m_unit->m_stream_id, 0);
 
        ret = sendStatus(node, &av_status_play_start, 
            &rtmp->m_unit->m_stream_name);
        if (0 != ret) { 
            break;
        } 

        m_stream_center->playCache(rtmp->m_unit);

        LOG_INFO("deal_play| txn=%.2f| stream_id=%u|"
            " app=%.*s| stream_name=%.*s|"
            " user_id=%u| msg=ok|", 
            *txn, rtmp->m_unit->m_stream_id,
            rtmp->m_unit->m_app.m_size,
            rtmp->m_unit->m_app.m_data,
            rtmp->m_unit->m_stream_name.m_size,
            rtmp->m_unit->m_stream_name.m_data,
            rtmp->m_unit->m_user_id);

        return 0;
    } while (0);

    LOG_ERROR("deal_play| ret=%d| txn=%.2f| stream_id=%u|"
        " app=%.*s| stream_name=%.*s|"
        " user_id=%u| msg=error|", 
        ret, *txn, rtmp->m_unit->m_stream_id,
        rtmp->m_unit->m_app.m_size,
        rtmp->m_unit->m_app.m_data,
        rtmp->m_unit->m_stream_name.m_size,
        rtmp->m_unit->m_stream_name.m_data,
        rtmp->m_unit->m_user_id);
    
    return ret;
}

Void RtmpHandler::notifyPublisher(RtmpUint* player, AMFObject* obj) {
    RtmpUint* publisher = NULL;
    
    if (!player->m_is_publisher && NULL != player->m_ctx) {
        publisher = player->m_ctx->m_publisher;

        if (NULL != publisher) { 
            sendAmfObj("test_pause", publisher->m_entity, 
                0, publisher->m_stream_id, 
                RTMP_MSG_TYPE_INVOKE, obj);
        }
    }
}

Int32 RtmpHandler::dealPause(RtmpNode* node, Rtmp* rtmp, AMFObject* obj) {
    Int32 ret = 0;
    double* txn = NULL;
    Int32* pi = NULL;

    do {
        txn = m_amf->indexPropNum(obj, 1);
        
        pi = m_amf->indexPropBool(obj, 3);
        if (NULL != pi) {
            rtmp->m_unit->m_is_pause = *pi;
        } else {
            ret = -1;
            break;
        }

        if (rtmp->m_unit->m_is_pause) {            
            ret = sendStatus(node, &av_netstream_pause_notify, 
                &rtmp->m_unit->m_stream_name); 
            
            /* user control stop stream */
            sendUsrCtrl(node, 1, rtmp->m_unit->m_stream_id, 0); 
        } else {
            /* user control begin stream */
            sendUsrCtrl(node, 0, rtmp->m_unit->m_stream_id, 0); 

            ret = sendStatus(node, &av_netstream_unpause_notify, 
                &rtmp->m_unit->m_stream_name); 

            m_stream_center->playCache(rtmp->m_unit);
        }
    } while (0);

    LOG_INFO("deal_pause| ret=%d| txn=%.2f| stream_id=%u|"
        " stream_name=%.*s| user_id=%u|", 
        ret, *txn, rtmp->m_unit->m_stream_id,
        rtmp->m_unit->m_stream_name.m_size,
        rtmp->m_unit->m_stream_name.m_data,
        rtmp->m_unit->m_user_id);

    return ret;
}

Int32 RtmpHandler::dealPublish(RtmpNode* node, Rtmp* rtmp, AMFObject* obj) {
    Int32 ret = 0;
    Bool bOk = FALSE;
    double* txn = NULL;
    Chunk* pv = NULL;

    do {
        txn = m_amf->indexPropNum(obj, 1);
        
        pv = m_amf->indexPropString(obj, 3); 
        if (NULL != pv) {
            creatAV(pv, &rtmp->m_unit->m_stream_name);
        } else {
            ret = -1;
            break;
        }
                
        pv = m_amf->indexPropString(obj, 4);
        if (NULL != pv) {
            creatAV(pv, &rtmp->m_unit->m_stream_type);
        } else {
            ret = -1;
            break;
        }
        
        genStreamKey(&rtmp->m_unit->m_app, &rtmp->m_unit->m_stream_name,
            &rtmp->m_unit->m_key);

        rtmp->m_unit->m_is_publisher = TRUE;

        bOk = m_stream_center->regPublisher(rtmp->m_unit);
        if (!bOk) { 
            ret = -1;
            break;
        }

        rtmp->m_status = ENUM_RTMP_STATUS_AUTH_OPER;

        /* begin stream */
        sendUsrCtrl(node, 0, rtmp->m_unit->m_stream_id, 0); 
 
        ret = sendStatus(node, &av_netstream_publish_start, 
            &rtmp->m_unit->m_stream_name); 
        if (0 != ret) {
            break;
        } 

        LOG_INFO("deal_publish| txn=%.2f| stream_id=%u|"
            " app=%.*s| stream_name=%.*s|"
            " user_id=%u| msg=ok|", 
            *txn, rtmp->m_unit->m_stream_id,
            rtmp->m_unit->m_app.m_size,
            rtmp->m_unit->m_app.m_data,
            rtmp->m_unit->m_stream_name.m_size,
            rtmp->m_unit->m_stream_name.m_data,
            rtmp->m_unit->m_user_id);

        return 0;
    } while (0);

    LOG_ERROR("deal_publish| ret=%d| txn=%.2f| stream_id=%u|"
        " app=%.*s| stream_name=%.*s|"
        " user_id=%u| msg=error|", 
        ret, *txn, rtmp->m_unit->m_stream_id,
        rtmp->m_unit->m_app.m_size,
        rtmp->m_unit->m_app.m_data,
        rtmp->m_unit->m_stream_name.m_size,
        rtmp->m_unit->m_stream_name.m_data,
        rtmp->m_unit->m_user_id);
    
    return ret;
}

Int32 RtmpHandler::dealUnpublish(RtmpNode*, Rtmp* rtmp, 
    AMFObject* obj) {
    Int32 ret = 0;
    double* txn = NULL;
    Chunk* pv = NULL;
    RtmpUint* unit = rtmp->m_unit;
    Chunk name = AMF_CHUNK_EMPTY;

    do {
        txn = m_amf->indexPropNum(obj, 1);
        
        pv = m_amf->indexPropString(obj, 3); 
        if (NULL != pv) {
            name = *pv;
        } else {
            ret = -1;
            break;
        } 

        LOG_INFO("deal_unpublish| txn=%.2f|"
            " stream_id=%u| name=%.*s| msg=ok|", 
            *txn, unit->m_stream_id,
            name.m_size, name.m_data);

        return 0;
    } while (0);
    
    LOG_INFO("deal_unpublish| ret=%d| txn=%.2f| stream_id=%u|"
        " name=%.*s| msg=error|", 
        ret, *txn, unit->m_stream_id,
        name.m_size, name.m_data);

    return ret;
}

Int32 RtmpHandler::dealDelStream(RtmpNode* , Rtmp* rtmp, AMFObject* obj) {
    Int32 ret = 0;
    double req_id = 0;
    double* txn = NULL;
    double* pf = NULL;
    RtmpUint* unit = rtmp->m_unit;

    do {
        txn = m_amf->indexPropNum(obj, 1);
        
        pf = m_amf->indexPropNum(obj, 3); 
        if (NULL != pf) {
            req_id = *pf;
        } else {
            ret = -1;
            break;
        } 

        /* notify the player that the stream is end */
        if (rtmp->m_unit->m_is_publisher) {
            
            m_stream_center->notifyEnd(unit->m_ctx);

            m_stream_center->unregPublisher(unit);
        } else {
            m_stream_center->unregPlayer(unit); 
        }
        
        rtmp->m_status = ENUM_RTMP_STATUS_CLOSED; 

        LOG_INFO("deal_deleteStream| txn=%.2f|"
            " stream_id=%u| req_id=%.2f| msg=ok|", 
            *txn, rtmp->m_unit->m_stream_id, req_id);

        return 0;
    } while (0);
    
    LOG_INFO("deal_deleteStream| ret=%d| txn=%.2f| stream_id=%u|"
        " req_id=%.2f| msg=error|", 
        ret, *txn, rtmp->m_unit->m_stream_id, req_id);

    return ret;
}

Int32 RtmpHandler::dealMetaData(RtmpNode* node, Rtmp* rtmp, CacheHdr* hdr) {
    Int32 ret = 0;
    Int32 cnt = 0;
    Bool bOk = FALSE;
    Cache* cache = NULL;
    RtmpPkg* pkg = NULL;
    Chunk chunk = AMF_CHUNK_EMPTY;
    AMFObject obj = AMF_OBJ_INVALID;

    cache = CacheCenter::getCache(hdr);
    pkg = MsgCenter::cast<RtmpPkg>(cache); 

    if (!pkg->m_has_striped) {
        cnt = m_amf->peekString(pkg->m_payload, pkg->m_size, &chunk);
        if (0 >= cnt) {
            LOG_ERROR("deal_meta_data| msg=decode string error|");

            return -1;
        }

        // strip '@metadata'
        if (matchAV(&av_meta_prefix_data, &chunk)) {
            pkg->m_skip = cnt;

            LOG_INFO("deal_meta_data| skip=%d|", pkg->m_skip);
        }

        pkg->m_has_striped = TRUE;

        bOk = m_amf->decode(&obj, pkg->m_payload + pkg->m_skip, 
            pkg->m_size - pkg->m_skip);
        if (bOk) {
            m_amf->dump("deal_meta_data", &obj); 
            
            m_amf->resetObj(&obj);
        } else {
            LOG_ERROR("deal_meta_data| msg=decode error|");

            m_amf->resetObj(&obj);
            return -1;
        } 
    }

    /* go to flv data handler */
    ret = dealFlvData(node, rtmp, ENUM_MSG_RTMP_TYPE_META_INFO, hdr);
    return ret;
}

Void RtmpHandler::cacheAvc(RtmpStream* stream, 
    Int32 msg_type, Cache* cache) {
    Int32 payload_size = 0;
    Byte* data = NULL;
    RtmpPkg* pkg = NULL;

    pkg = MsgCenter::cast<RtmpPkg>(cache);
    
    payload_size = pkg->m_size - pkg->m_skip;
    data = pkg->m_payload + pkg->m_skip;

    switch (msg_type) {
    case ENUM_MSG_RTMP_TYPE_AUDIO:
        if (2 <= payload_size && m_stream_center->chkAudioSeq(data)) {
            m_stream_center->cacheAvc(stream, ENUM_AVC_AUDIO, cache);
        } else {
            m_stream_center->updateAvc(stream, ENUM_AVC_AUDIO, cache);
        }

        m_stream_center->updateAvc(stream, ENUM_AVC_META_DATA, cache);
        
        break;

    case ENUM_MSG_RTMP_TYPE_VIDEO:
        if (2 <= payload_size && m_stream_center->chkVideoSeq(data)) {
            m_stream_center->cacheAvc(stream, ENUM_AVC_VEDIO, cache);
        } else {
            m_stream_center->updateAvc(stream, ENUM_AVC_VEDIO, cache);
        }
        
        break;

    case ENUM_MSG_RTMP_TYPE_META_INFO:
        m_stream_center->cacheAvc(stream, ENUM_AVC_META_DATA, cache);
        break;

    default:
        break;
    }
}

Int32 RtmpHandler::dealFlvData(RtmpNode* node, Rtmp* rtmp,
    Int32 msg_type, CacheHdr* hdr) {
    Int32 ret = 0;
    Cache* cache = NULL;
    RtmpUint* unit = rtmp->m_unit;
    
    cache = CacheCenter::getCache(hdr); 
    
    if (unit->m_is_publisher) {
        
        /* cache some data */
        cacheAvc(unit->m_ctx, msg_type, cache);

        if (!unit->m_is_pause) {
            m_stream_center->publish(unit->m_ctx, msg_type, cache); 
        }
    } else {
        if (!unit->m_is_pause) {
            ret = sendRtmpPkg(node, unit->m_stream_id, cache); 
        }
    } 

    LOG_DEBUG("deal_flv_data| msg_type=%d| user_id=%u|"
        " stream_id=%u| is_publisher=%d| msg=ok|", 
        msg_type, 
        unit->m_user_id,
        unit->m_stream_id,
        unit->m_is_publisher); 
    return ret;
}

Void RtmpHandler::closeRtmp(RtmpNode*, Rtmp* rtmp) {
    if (ENUM_RTMP_STATUS_AUTH_OPER == rtmp->m_status) {
        if (rtmp->m_unit->m_is_publisher) {
            m_stream_center->unregPublisher(rtmp->m_unit);
        } else {
            m_stream_center->unregPlayer(rtmp->m_unit);
        }
    }
}

Void RtmpHandler::updateOutTs(RtmpUint* unit) {
    if (unit->m_base_out_ts < unit->m_max_out_epoch) {
        unit->m_base_out_ts = unit->m_max_out_epoch;
    }
}

Int32 RtmpHandler::dealNotifyEndStream(RtmpNode* ,
    Rtmp* rtmp, CacheHdr* hdr) {
    Int32 ret = 0;
    Uint32 req_id = 0;
    RtmpUint* unit = rtmp->m_unit;

    req_id = (Uint32)MsgCenter::getExchMsgVal(hdr);

    /* when a stream ends, then the players base this epoch */
    updateOutTs(unit);

    LOG_INFO("deal_notify_end| stream_id=%u| self_id=%u|"
        " base_out_ts=%u|",
        req_id, unit->m_stream_id, unit->m_base_out_ts);

    return ret;
}

