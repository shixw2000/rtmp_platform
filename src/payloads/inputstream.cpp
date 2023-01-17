#include"inputstream.h"
#include"datatype.h"
#include"cache.h"
#include"rtmpnode.h"
#include"analyser.h"
#include"msgtype.h"


const RtmpInput::PFunc RtmpInput::m_funcs[ENUM_RTMP_RD_END] = {
    &RtmpInput::readInit,
    &RtmpInput::readC0C1,
    &RtmpInput::readC2,
    &RtmpInput::readBasic,
    &RtmpInput::readHeader,
    &RtmpInput::readExtTime,
    &RtmpInput::readChunk,
};

Int32 RtmpInput::parseMsg(Rtmp* rtmp, Byte* data, Int32 len) {
    Int32 ret = 0;
    Chunk chunk = {data, len};

    while (0 < chunk.m_size && 0 == ret) {
        ret = (this->*(m_funcs[rtmp->m_chn.m_rd_stat]))(rtmp, &chunk);
    }

    return ret;
}

Bool RtmpInput::fill(RtmpChn* chn, Chunk* src) {
    Int32 needlen = chn->m_size - chn->m_upto;

    if (needlen <= src->m_size) {    
        memcpy(chn->m_data + chn->m_upto, src->m_data, needlen); 

        chn->m_upto += needlen;
        
        src->m_data += needlen;
        src->m_size -= needlen;

        return TRUE;
    } else {
        memcpy(chn->m_data + chn->m_upto, src->m_data, src->m_size);

        chn->m_upto += src->m_size;
        
        src->m_data += src->m_size;
        src->m_size = 0;

        return FALSE;
    } 
}

Void RtmpInput::resetChnCache(RtmpChn* chn) { 
    if (NULL != chn->m_cache) {
        CacheCenter::del(chn->m_cache);
        
        chn->m_cache = NULL;
        
        chn->m_data = NULL;
        chn->m_upto = 0;
        chn->m_size = 0; 
    }
}

Void RtmpInput::resetChnPkg(RtmpChn* chn) { 
    if (NULL != chn->m_input && NULL != chn->m_input->m_pkg) {
        CacheCenter::del(chn->m_input->m_pkg);
        
        chn->m_input->m_pkg = NULL;

        chn->m_input->m_rd_bytes = 0;
        chn->m_data = NULL;
        chn->m_upto = 0;
        chn->m_size = 0; 
    }
}

Int32 RtmpInput::readInit(Rtmp* rtmp, Chunk*) {
    Int32 size = RTMP_SIG_SIZE + 1; //c01 size
    CommPkg* pkg = NULL;
    RtmpChn* chn = &rtmp->m_chn; 
   
    chn->m_cache = MsgCenter::creatCache<CommPkg>(size); 
    if (NULL != chn->m_cache) {
        pkg = MsgCenter::cast<CommPkg>(chn->m_cache);
        pkg->m_size = size;
        
        chn->m_data = pkg->m_data; 
        chn->m_size = pkg->m_size;
        
        /* read c01 */
        chn->m_rd_stat = ENUM_RTMP_RD_C0C1;
        return 0;
    } else { 
        return -1;
    }
}

Int32 RtmpInput::readC0C1(Rtmp* rtmp, Chunk* chunk) {
    Int32 ret = 0;
    Int32 size = RTMP_SIG_SIZE; // c2 size
    Bool bOk = FALSE;
    CommPkg* pkg = NULL;
    RtmpNode* node = rtmp->m_entity;
    RtmpChn* chn = &rtmp->m_chn;

    bOk = fill(chn, chunk);
    if (bOk) {        
        ret = node->recv(node, ENUM_MSG_RTMP_HANDSHAKE_C0C1, 
            RTMP_INVALID_SID, chn->m_cache); 
        
        /* send and then release */
        resetChnCache(chn);
        
        if (0 == ret) { 
            chn->m_cache = MsgCenter::creatCache<CommPkg>(size); 
            if (NULL != chn->m_cache) {
                pkg = MsgCenter::cast<CommPkg>(chn->m_cache);
                pkg->m_size = size;
                
                chn->m_data = pkg->m_data; 
                chn->m_size = pkg->m_size;

                /* read c2 */
                chn->m_rd_stat = ENUM_RTMP_RD_C2;
                return 0;
            } else {
                return -1;
            }
        } else { 
            return ret;
        }
    } else {
        return 0;
    } 
}

Int32 RtmpInput::readC2(Rtmp* rtmp, Chunk* chunk) {
    Int32 ret = 0;
    Bool bOk = FALSE;
    RtmpNode* node = rtmp->m_entity;
    RtmpChn* chn = &rtmp->m_chn;
    
    bOk = fill(chn, chunk);
    if (bOk) {
        ret = node->recv(node, ENUM_MSG_RTMP_HANDSHAKE_C2, 
            RTMP_INVALID_SID, chn->m_cache); 
        
        /* send and then release */
        resetChnCache(chn);
        
        if (0 == ret) {

            /* start to read basic header */
            chn->m_rd_stat = ENUM_RTMP_RD_BASIC;
            return 0;
        } else { 
            return ret;
        }
    } else {
        return 0;
    }
}

Int32 RtmpInput::readBasic(Rtmp* rtmp, Chunk* chunk) {
    Int32 ret = 0;
    Uint32 fmt = 0;
    Uint32 cid = 0;
    Byte b = 0;
    RtmpPkg* pkg = NULL;
    RtmpChn* chn = &rtmp->m_chn;
    
    b = chunk->m_data[0];
    ++chunk->m_data;
    --chunk->m_size;
    
    fmt = (b >> 6);
    cid = (b & 0x3F);

    if (!rtmp->m_chnnIn[cid]) {
        rtmp->m_chnnIn[cid] = creatChnIn(cid);
    }

    /* set current chn */
    chn->m_input = rtmp->m_chnnIn[cid];
    
    /* this fmt is record by prev header */
    chn->m_input->m_fmt = fmt;
    
    if (RTMP_MINIMUM_FMT > fmt) {
        chn->m_data = chn->m_buff;
        chn->m_upto = 0;
        chn->m_size = RTMP_HEADER_SIZE_ARR[fmt];
        
        chn->m_rd_stat = ENUM_RTMP_RD_HEADER;
        return 0;
    } else { 
    
        if (NULL == chn->m_input->m_pkg) {
            /* it is a new pkg */
            chn->m_input->m_pkg = creatPkg(chn->m_input->m_rtmp_type, 
                chn->m_input->m_pld_size);

            pkg = MsgCenter::cast<RtmpPkg>(chn->m_input->m_pkg);
            pkg->m_timestamp = chn->m_input->m_timestamp;
            pkg->m_epoch = chn->m_input->m_epoch + chn->m_input->m_timestamp;
            pkg->m_sid = chn->m_input->m_sid;
            
            chn->m_input->m_rd_bytes = 0;
        } else {
            /* it is partial of a pkg */
        }
        
        chn->m_size = 0; 
        ret = begChunk(rtmp);
        return ret; 
    } 
}

Int32 RtmpInput::readHeader(Rtmp* rtmp, Chunk* chunk) {
    Int32 ret = 0;
    Bool bOk = FALSE;
    RtmpPkg* pkg = NULL;
    RtmpChn* chn = &rtmp->m_chn;

    bOk = fill(chn, chunk);
    if (bOk) {
        parseHdr(rtmp);

        pkg = MsgCenter::cast<RtmpPkg>(chn->m_input->m_pkg); 

        if (RTMP_TIMESTAMP_EXT_MARK != pkg->m_timestamp) {

            if (RTMP_LARGE_FMT != chn->m_input->m_fmt) {
                pkg->m_epoch = chn->m_input->m_epoch + pkg->m_timestamp;
            } else {
                pkg->m_epoch = pkg->m_timestamp;
            }

            ret = begChunk(rtmp);
            return ret;
        } else {
            /* read a extend ts */
            chn->m_data = chn->m_buff;
            chn->m_upto = 0;
            chn->m_size = RTMP_EXT_TIMESTAMP_SIZE;
      
            chn->m_rd_stat = ENUM_RTMP_RD_EXT_TIME; 
            return 0;
        }
    } else {
        return 0;
    }
}

Void RtmpInput::parseHdr(Rtmp* rtmp) {
    Byte rtmp_type = 0;
    Uint32 ts = 0;
    Uint32 sid = 0;
    Uint32 payload_size = 0;
    RtmpPkg* pkg = NULL;
    RtmpChn* chn = &rtmp->m_chn;
    Parser parser(chn->m_data, chn->m_size);

    switch (chn->m_input->m_fmt) {
    case RTMP_LARGE_FMT:
        parser.parse24(&ts);
        parser.parse24(&payload_size);
        parser.parse8(&rtmp_type);
        parser.parse32LE(&sid);
        break;
        
    case RTMP_MEDIUM_FMT:
        parser.parse24(&ts);
        parser.parse24(&payload_size);
        parser.parse8(&rtmp_type);
        sid = chn->m_input->m_sid;
        break;

    case RTMP_SMALL_FMT:
    default:
        parser.parse24(&ts);
        payload_size = chn->m_input->m_pld_size;
        rtmp_type = chn->m_input->m_rtmp_type;
        sid = chn->m_input->m_sid;
        break; 
    } 

    /* for a new package */
    if (NULL == chn->m_input->m_pkg) {
        chn->m_input->m_pkg = creatPkg(rtmp_type, payload_size);
        chn->m_input->m_rd_bytes = 0;
    } else {
        /* ignore old package */
        CacheCenter::del(chn->m_input->m_pkg);

        chn->m_input->m_pkg = creatPkg(rtmp_type, payload_size);
        chn->m_input->m_rd_bytes = 0;
    }

    pkg = MsgCenter::cast<RtmpPkg>(chn->m_input->m_pkg); 
    pkg->m_timestamp = ts;
    pkg->m_sid = sid;
    return;
}

Int32 RtmpInput::readExtTime(Rtmp* rtmp, Chunk* chunk) {
    Int32 ret = 0;
    Bool bOk = FALSE;
    RtmpChn* chn = &rtmp->m_chn;

    bOk = fill(chn, chunk);
    if (bOk) {
        parseExtTime(rtmp);

        ret = begChunk(rtmp);
        return ret;
    } else {
        return 0;
    }
}

Void RtmpInput::parseExtTime(Rtmp* rtmp) {
    Uint32 ts = 0;
    RtmpPkg* pkg = NULL;
    RtmpChn* chn = &rtmp->m_chn;
    Parser parser(chn->m_data, chn->m_size);

    parser.parse32(&ts);

    pkg = MsgCenter::cast<RtmpPkg>(chn->m_input->m_pkg); 

    pkg->m_timestamp = ts; 
    if (RTMP_LARGE_FMT != chn->m_input->m_fmt) {
        pkg->m_epoch = chn->m_input->m_epoch + ts;
    } else {
        pkg->m_epoch = ts;
    }
}

Int32 RtmpInput::begChunk(Rtmp* rtmp) {
    Int32 ret = 0;
    Int32 rdlen = 0;
    RtmpPkg* pkg = NULL;
    RtmpChn* chn = &rtmp->m_chn;
    
    pkg = MsgCenter::cast<RtmpPkg>(chn->m_input->m_pkg);
    
    if (chn->m_input->m_rd_bytes < pkg->m_size) {
        /* start to read chunk */
        chn->m_data = pkg->m_payload + chn->m_input->m_rd_bytes;
        chn->m_upto = 0;
        
        rdlen = pkg->m_size - chn->m_input->m_rd_bytes;
        if (rdlen > rtmp->m_chunkSizeIn) {
            chn->m_size = rtmp->m_chunkSizeIn;
        } else {
            chn->m_size = rdlen;
        } 
        
        chn->m_rd_stat = ENUM_RTMP_RD_CHUNK; 
        return 0;
    } else {
        chn->m_data = NULL;
        chn->m_upto = 0;
        chn->m_size = 0;
        
        /* empty payload */
        ret = endChunk(rtmp); 
        return ret;
    }
}

Int32 RtmpInput::readChunk(Rtmp* rtmp, Chunk* chunk) {
    Int32 ret = 0;
    Bool bOk = FALSE;
    RtmpChn* chn = &rtmp->m_chn;

    bOk = fill(chn, chunk);
    if (bOk) {
        ret = endChunk(rtmp); 
        
        return ret;
    } else {
        return 0;
    }
}

Int32 RtmpInput::endChunk(Rtmp* rtmp) {
    Int32 ret = 0;
    RtmpPkg* pkg = NULL;
    RtmpNode* node = rtmp->m_entity;
    RtmpChn* chn = &rtmp->m_chn; 

    pkg = MsgCenter::cast<RtmpPkg>(chn->m_input->m_pkg);
    
    chn->m_input->m_rd_bytes += chn->m_size; 
    
    if (chn->m_input->m_rd_bytes == pkg->m_size) { 
        LOG_INFO("end_chunk_pkg| fmt=%u| cid=%u|"
            " epoch=%u| timestamp=%u| pkg_size=%d|"
            " rtmp_type=%u| sid=%u|"
            " chunk_size=%d| chunk_in_size=%d|",
            chn->m_input->m_fmt,
            chn->m_input->m_cid,
            
            pkg->m_epoch,
            pkg->m_timestamp,
            pkg->m_size, 
            
            pkg->m_rtmp_type,
            pkg->m_sid,
            
            chn->m_size,
            rtmp->m_chunkSizeIn);

        /* receive a completed msg */
        node->recv(node, pkg->m_msg_type, pkg->m_sid, chn->m_input->m_pkg); 

        /* updata prev header */
        chn->m_input->m_epoch = pkg->m_epoch;
        chn->m_input->m_timestamp = pkg->m_timestamp;
        chn->m_input->m_sid = pkg->m_sid;
        chn->m_input->m_rtmp_type = pkg->m_rtmp_type;
        chn->m_input->m_pld_size = (Uint32)pkg->m_size; 

        /* release parameters */
        resetChnPkg(chn);
    } else {
        LOG_DEBUG("end_chunk| fmt=%u| cid=%u|"
            " epoch=%u| timestamp=%u| payload_size=%u|"
            " rtmp_type=%u| sid=%u|"
            " rd_bytes=%d| chunk_size=%d| chunk_in_size=%d|",
            chn->m_input->m_fmt,
            chn->m_input->m_cid, 

            pkg->m_epoch,
            pkg->m_timestamp,
            pkg->m_size, 
            
            pkg->m_rtmp_type,
            pkg->m_sid,
            
            chn->m_input->m_rd_bytes,
            chn->m_size, 
            rtmp->m_chunkSizeIn);
            
        chn->m_data = NULL;
        chn->m_upto = 0;
        chn->m_size = 0; 
    }

    chn->m_input = NULL;
    chn->m_rd_stat = ENUM_RTMP_RD_BASIC; 
    return ret;
}

