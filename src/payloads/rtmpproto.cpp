#include"rtmpproto.h"
#include"rtmpnode.h"
#include"cache.h"
#include"analyser.h"


Cache* RtmpProto::genCall(const Chunk* call, const double* txn, 
    const AMFObject* info, AMFDataType arg_type, const Void* arg) {
    Bool bOk = FALSE;
    Cache* cache = NULL;
    RtmpPkg* pkg = NULL;
    AMFObject obj = AMF_OBJ_INVALID;
    Chunk chunk = AMF_CHUNK_EMPTY;
    
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

    bOk = m_amf->encode(&obj, &chunk);
    if (bOk) {
        m_amf->dump("gen_call", &obj);
        
        cache = creatPkg(RTMP_MSG_TYPE_INVOKE, chunk.m_size);
        if (NULL != cache) { 
            pkg = MsgCenter::cast<RtmpPkg>(cache); 

            CacheCenter::copy(pkg->m_payload, chunk.m_data, chunk.m_size); 
        } 
    }

    /* just release obj itself */
    m_amf->resetObj(&obj); 
    return cache;
    
}

Cache* RtmpProto::genStatus(const Chunk* status, const Chunk* path,
    const Char* desc) {
    Int32 len = 0;
    Cache* cache = NULL;
    Chunk chunk = AMF_CHUNK_EMPTY;
    AMFObject arg = AMF_OBJ_INVALID;

    len = strnlen(desc, 64);
    chunk.m_data = (Byte*)desc;
    chunk.m_size = len; 
        
    m_amf->addPropAny(&arg, &av_level, AMF_STRING, &av_status_level_ok);
    m_amf->addPropAny(&arg, &av_code, AMF_STRING, status);
    m_amf->addPropAny(&arg, &av_description, AMF_STRING, &chunk);
    m_amf->addPropAny(&arg, &av_details, AMF_STRING, path);
    m_amf->addPropAny(&arg, &av_clientid, AMF_STRING, &av_def_clientid); 

    cache = genCall(&av_onStatus, &AMF_ZERO_DOUBLE, NULL, AMF_OBJECT, &arg);

    /* release arg */
    m_amf->freeObj(&arg); 
    return cache;
}

Cache* RtmpProto::genResult(const double* txn, const AMFObject* info,
    AMFDataType arg_type, const Void* arg) {
    Cache* cache = NULL;

    cache = genCall(&av__result, txn, info, arg_type, arg); 
    return cache;
}

Int32 RtmpProto::recvCmd(Rtmp* rtmp, Int32 msg_type, Int64 val) {
    Int32 ret = 0;
    RtmpNode* node = rtmp->m_entity;

    ret = node->recvCmd(node, msg_type, val);
    return ret;
}

Int32 RtmpProto::recv(RtmpUint* unit, Int32 msg_type, Cache* cache) {
    Int32 ret = 0;
    RtmpNode* node = unit->m_parent->m_entity;

    ret = node->recv(node, msg_type, unit->m_sid, cache);
    return ret;
} 

Int32 RtmpProto::send(RtmpUint* unit, Uint32 delta_ts, Cache* cache) {
    Int32 ret = 0;
    RtmpNode* node = unit->m_parent->m_entity;

    ret = node->sendPkg(node, unit->m_sid, delta_ts, cache);
    return ret;
}

Int32 RtmpProto::sendCtrl(Rtmp* rtmp, Uint32 rtmp_type, const Chunk* chunk) {
    Int32 ret = 0;
    RtmpNode* node = rtmp->m_entity;

    ret = node->sendPayload(node, RTMP_INVALID_SID, rtmp_type, chunk);
    return ret;
}

Int32 RtmpProto::sendChunkSize(Rtmp* rtmp, Uint32 chunkSize) {
    Int32 ret = 0;
    Byte buff[RTMP_MIN_CHUNK_SIZE] = {0};
    Chunk chunk = {buff, RTMP_MIN_CHUNK_SIZE};
    Builder builder(&chunk); 
    
    builder.build32(&chunkSize);
    chunk.m_size = builder.used();
    chunk.m_data = buff;

    ret = sendCtrl(rtmp, RTMP_MSG_TYPE_CHUNK_SIZE, &chunk);
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

Int32 RtmpProto::sendBytesReport(Rtmp* rtmp, Uint32 seqBytes) {
    Int32 ret = 0;
    Byte buff[RTMP_MIN_CHUNK_SIZE] = {0};
    Chunk chunk = {buff, RTMP_MIN_CHUNK_SIZE};
    Builder builder(&chunk); 

    builder.build32(&seqBytes);

    chunk.m_size = builder.used();
    chunk.m_data = buff;
    
    ret = sendCtrl(rtmp, RTMP_MSG_TYPE_BYTES_READ_REPORT, &chunk);
    if (0 == ret) { 
        LOG_INFO("send_bytes_report| ack_rcv_bytes=%u| msg=send amf ok|", 
            seqBytes); 
    } else {
        LOG_ERROR("send_bytes_report| ack_rcv_bytes=%u| msg=send amf error|", 
            seqBytes);
    }
  
    return ret;
}

Int32 RtmpProto::sendServBw(Rtmp* rtmp, Uint32 bw) {
    Int32 ret = 0;
    Byte buff[RTMP_MIN_CHUNK_SIZE] = {0};
    Chunk chunk = {buff, RTMP_MIN_CHUNK_SIZE};
    Builder builder(&chunk); 

    builder.build32(&bw);

    chunk.m_size = builder.used();
    chunk.m_data = buff; 

    ret = sendCtrl(rtmp, RTMP_MSG_TYPE_SERVER_BW, &chunk);
    if (0 == ret) { 
        LOG_INFO("send_serv_bw| snd_window_size=%u| msg=send amf ok|", bw);
        
        /* update send window size */
        rtmp->m_srv_window_size = bw;
    } else {
        LOG_ERROR("send_serv_bw| snd_window_size=%u| msg=send amf error|", bw);
    }

    return ret;
}

Int32 RtmpProto::sendCliBw(Rtmp* rtmp, Uint32 bw, Int32 limit){
    Int32 ret = 0;
    Byte ch = (Byte)limit;
    Byte buff[RTMP_MIN_CHUNK_SIZE] = {0};
    Chunk chunk = {buff, RTMP_MIN_CHUNK_SIZE};
    Builder builder(&chunk); 

    builder.build32(&bw);
    builder.build8(&ch);

    chunk.m_size = builder.used();
    chunk.m_data = buff; 

    ret = sendCtrl(rtmp, RTMP_MSG_TYPE_CLIENT_BW, &chunk); 
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

Int32 RtmpProto::sendUsrCtrl(Rtmp* rtmp, Uint16 ev, Uint32 p1, Uint32 p2) {
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

    ret = sendCtrl(rtmp, RTMP_MSG_TYPE_USR_CTRL_MSG, &chunk); 
    if (0 == ret) { 
        LOG_INFO("send_usr_ctrl| ev=%u| data=%u| extra=%u| msg=send amf ok|", 
            ev, p1, p2);
    } else {
        LOG_ERROR("send_usr_ctrl| ev=%u| data=%u| extra=%u| msg=send amf error|",
            ev, p1, p2);
    }
    
    return ret;
}

Int32 RtmpProto::sendResult(Rtmp* rtmp, Uint32 sid, const double* txn, 
    const AMFObject* info, AMFDataType arg_type, const Void* arg) {
    Int32 ret = 0;
    Cache* cache = NULL;
    RtmpNode* node = rtmp->m_entity;

    cache = genResult(txn, info, arg_type, arg);
    if (NULL != cache) {
        ret = node->sendPkg(node, sid, 0, cache);

        /* release cache itself */
        CacheCenter::del(cache);
    } else {
        ret = -1;
    }

    return ret;
} 

Int32 RtmpProto::sendStatus(Rtmp* rtmp, Uint32 sid,
    const Chunk* status, const Chunk* path,
    const Char* desc) {
    Int32 ret = 0;
    Cache* cache = NULL;
    RtmpNode* node = rtmp->m_entity;

    cache = genStatus(status, path, desc);
    if (NULL != cache) {
        ret = node->sendPkg(node, sid, 0, cache);

        /* release cache itself */
        CacheCenter::del(cache); 
    } else {
        ret = -1;
    }

    return ret;
}

Int32 RtmpProto::sendCall(Rtmp* rtmp, Uint32 sid, 
    const Chunk* call, const double* txn,
    const AMFObject* info, 
    AMFDataType arg_type, const Void* arg) {
    Int32 ret = 0;
    Cache* cache = NULL;
    RtmpNode* node = rtmp->m_entity;

    cache = genCall(call, txn, info, arg_type, arg);
    if (NULL != cache) {
        ret = node->sendPkg(node, sid, 0, cache);

        /* release cache itself */
        CacheCenter::del(cache);
    } else {
        ret = -1;
    }

    return ret;
}

Void RtmpProto::notifyStart(Rtmp* rtmp, Uint32 sid, 
    const Chunk* code, const Char* desc) {
    RtmpUint* unit = rtmp->m_unit[sid];
    
    if (NULL != unit) {
        /* user control begin stream */
        sendUsrCtrl(rtmp, ENUM_USR_CTRL_STREAM_BEGIN, sid);
        sendStatus(rtmp, sid, code, &unit->m_stream_name, desc); 
    }
}

Void RtmpProto::notifyStop(Rtmp* rtmp, Uint32 sid, 
    const Chunk* code, const Char* desc) {
    RtmpUint* unit = rtmp->m_unit[sid];
    
    if (NULL != unit) {
        /* user control end stream */
        sendUsrCtrl(rtmp, ENUM_USR_CTRL_STREAM_END, sid);
        sendStatus(rtmp, sid, code, &unit->m_stream_name, desc); 
    }
}

