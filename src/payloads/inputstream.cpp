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

Int32 RtmpInput::parseMsg(RtmpNode* node, Rtmp* rtmp, 
    Byte* data, Int32 len) {
    Int32 ret = 0;
    Chunk chunk = {data, len};

    while (0 < chunk.m_size && 0 == ret) {
        ret = (this->*(m_funcs[rtmp->m_chn.m_rd_stat]))(node, rtmp, &chunk);
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

Int32 RtmpInput::readInit(RtmpNode*, Rtmp* rtmp, Chunk*) {
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

Int32 RtmpInput::readC0C1(RtmpNode* node, Rtmp* rtmp, Chunk* chunk) {
    Int32 ret = 0;
    Int32 size = RTMP_SIG_SIZE; // c2 size
    Bool bOk = FALSE;
    CommPkg* pkg = NULL;
    RtmpChn* chn = &rtmp->m_chn;

    bOk = fill(chn, chunk);
    if (bOk) {        
        ret = node->onRecv(node, ENUM_MSG_RTMP_HANDSHAKE_C0C1, chn->m_cache); 
        
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

Int32 RtmpInput::readC2(RtmpNode* node, Rtmp* rtmp, Chunk* chunk) {
    Int32 ret = 0;
    Bool bOk = FALSE;
    RtmpChn* chn = &rtmp->m_chn;
    
    bOk = fill(chn, chunk);
    if (bOk) {
        ret = node->onRecv(node, ENUM_MSG_RTMP_HANDSHAKE_C2, chn->m_cache); 
        
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

Int32 RtmpInput::readBasic(RtmpNode* node, Rtmp* rtmp, Chunk* chunk) {
    static const Int32 DEF_FMT_SIZE[] = { 11, 7, 3, 0 };
    Int32 ret = 0;
    Uint32 fmt = 0;
    Uint32 cid = 0;
    Byte b = 0;
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
    chn->m_input->m_header.m_fmt = fmt;
    
    if (0 < DEF_FMT_SIZE[fmt]) {
        chn->m_data = chn->m_buff;
        chn->m_upto = 0;
        chn->m_size = DEF_FMT_SIZE[fmt];
        
        chn->m_rd_stat = ENUM_RTMP_RD_HEADER;
    } else {
        ret = begChunk(node, rtmp);
    } 

    return ret;
}

Int32 RtmpInput::readHeader(RtmpNode* node, Rtmp* rtmp, Chunk* chunk) {
    Int32 ret = 0;
    Bool bOk = FALSE;
    RtmpChn* chn = &rtmp->m_chn;

    bOk = fill(chn, chunk);
    if (bOk) {
        parseHdr(rtmp);

        if (RTMP_TIMESTAMP_EXT_MARK != chn->m_input->m_header.m_timestamp) {
            ret = begChunk(node, rtmp);

            return ret;
        } else {
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
    Uint32 val = 0;
    Byte b = 0;
    RtmpChn* chn = &rtmp->m_chn;
    Parser parser(chn->m_data, chn->m_size);

    if (3 <= parser.available()) {
        parser.parse24(&val);

        chn->m_input->m_header.m_timestamp = val;
    } else {
        return;
    }

    if (4 <= parser.available()) {
        parser.parse24(&val);
        parser.parse8(&b);

        chn->m_input->m_header.m_pld_size = val;
        chn->m_input->m_header.m_type = b;

        /* reset payload */
        if (NULL != chn->m_input->m_pkg) {
            resetChnPkg(chn);
        }
    } else {
        return;
    }

    if (4 <= parser.available()) {
        parser.parse32LE(&val);

        chn->m_input->m_header.m_stream_id = val;
    } 

    return;
}

Int32 RtmpInput::readExtTime(RtmpNode* node, Rtmp* rtmp, Chunk* chunk) {
    Int32 ret = 0;
    Bool bOk = FALSE;
    RtmpChn* chn = &rtmp->m_chn;

    bOk = fill(chn, chunk);
    if (bOk) {
        parseExtTime(rtmp);

        ret = begChunk(node, rtmp);
        return ret;
    } else {
        return 0;
    }
}

Void RtmpInput::parseExtTime(Rtmp* rtmp) {
    Uint32 val = 0;
    RtmpChn* chn = &rtmp->m_chn;
    Parser parser(chn->m_data, chn->m_size);

    parser.parse32(&val);

    chn->m_input->m_header.m_timestamp = val; 
}

Int32 RtmpInput::begChunk(RtmpNode* node, Rtmp* rtmp) {
    Int32 ret = 0;
    Int32 rdlen = 0;
    RtmpPkg* pkg = NULL;
    RtmpChn* chn = &rtmp->m_chn;
    HeaderRtmp* header = &chn->m_input->m_header;

    chn->m_data = NULL;
    chn->m_upto = 0;
    chn->m_size = 0; 
    
    /* coming a new payload */
    if (NULL == chn->m_input->m_pkg) {
        
        /* read a absolute timestamp */
        if (RTMP_LARGE_FMT == header->m_fmt) {
            header->m_epoch = header->m_timestamp;
        } else {
            header->m_epoch += header->m_timestamp;
        }

        chn->m_input->m_pkg = creatPkg(header->m_epoch,
            header->m_stream_id, header->m_type, 
            header->m_pld_size);
        if (NULL != chn->m_input->m_pkg) {
            pkg = MsgCenter::cast<RtmpPkg>(chn->m_input->m_pkg);
            
            pkg->m_fmt = header->m_fmt;
            pkg->m_cid = header->m_cid;
            pkg->m_timestamp = header->m_timestamp;
        } else {
            return -1;
        }
    } else {
        pkg = MsgCenter::cast<RtmpPkg>(chn->m_input->m_pkg);
    }
    
    if (chn->m_input->m_rd_bytes < pkg->m_size) {
        /* start to read chunk */
        chn->m_data = pkg->m_payload + chn->m_input->m_rd_bytes;
        
        rdlen = pkg->m_size - chn->m_input->m_rd_bytes;
        if (rdlen > rtmp->m_chunkSizeIn) {
            chn->m_size = rtmp->m_chunkSizeIn;
        } else {
            chn->m_size = rdlen;
        } 
        
        chn->m_rd_stat = ENUM_RTMP_RD_CHUNK; 
        return 0;
    } else {
        /* empty payload */
        ret = endChunk(node, rtmp); 
        return ret;
    }
}

Int32 RtmpInput::readChunk(RtmpNode* node, Rtmp* rtmp, Chunk* chunk) {
    Int32 ret = 0;
    Bool bOk = FALSE;
    RtmpChn* chn = &rtmp->m_chn;

    bOk = fill(chn, chunk);
    if (bOk) {
        ret = endChunk(node, rtmp); 
        
        return ret;
    } else {
        return 0;
    }
}

Int32 RtmpInput::endChunk(RtmpNode* node, Rtmp* rtmp) {
    Int32 ret = 0;
    RtmpPkg* pkg = NULL;
    RtmpChn* chn = &rtmp->m_chn; 
    HeaderRtmp* header = &chn->m_input->m_header;

    pkg = MsgCenter::cast<RtmpPkg>(chn->m_input->m_pkg);
    
    chn->m_input->m_rd_bytes += chn->m_size; 
    
    if (chn->m_input->m_rd_bytes == pkg->m_size) { 
        LOG_INFO("end_chunk_pkg| fmt=%u| cid=%u|"
            " epoch=%u| timestamp=%u| pkg_size=%d|"
            " rtmp_type=%u| stream_id=%u|"
            " chunk_size=%d|",
            pkg->m_fmt,
            pkg->m_cid,
            
            pkg->m_epoch,
            pkg->m_timestamp,
            pkg->m_size, 
            
            pkg->m_rtmp_type,
            pkg->m_stream_id,
            
            chn->m_size);

        if (ENUM_MSG_RTMP_TYPE_CTRL != pkg->m_msg_type) { 
            ret = node->onRecv(node, pkg->m_msg_type, chn->m_input->m_pkg); 
        } else {
            ret = node->dealCtrl(node, chn->m_input->m_pkg);
        }

        resetChnPkg(chn);
        chn->m_input = NULL;
        
        if (0 == ret) {
            chn->m_rd_stat = ENUM_RTMP_RD_BASIC; 
            return 0;
        } else {
            return ret;
        }
    } else {
        LOG_DEBUG("end_chunk| fmt=%u| cid=%u|"
            " epoch=%u| timestamp=%u| payload_size=%u|"
            " rtmp_type=%u| stream_id=%u|"
            " pkg_size=%d| rd_bytes=%d| chunk_size=%d|",
            header->m_fmt,
            header->m_cid, 
            
            header->m_epoch,
            header->m_timestamp,
            header->m_pld_size, 
            
            header->m_type,
            header->m_stream_id,
            
            pkg->m_size,
            chn->m_input->m_rd_bytes,
            chn->m_size);
            
        chn->m_input = NULL;
        chn->m_data = NULL;
        chn->m_upto = 0;
        chn->m_size = 0;

        chn->m_rd_stat = ENUM_RTMP_RD_BASIC; 
        return 0;
    }
}

