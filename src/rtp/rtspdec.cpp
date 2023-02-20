#include<stdlib.h>
#include"rtspdec.h"
#include"rtspcenter.h"
#include"cache.h"
#include"rtspnode.h"


const RtspDec::PFunc RtspDec::m_funcs[ENUM_RTSP_DEC_END] = {
    &RtspDec::readInit,
    &RtspDec::readHead,
    &RtspDec::readBody,
};

Int32 RtspDec::parseMsg(Rtsp* rtsp, Byte* data, Int32 len) {
    Int32 ret = 0;
    AVal chunk = DEF_EMPTY_AVAL;

    chunk.m_data = data;
    chunk.m_size = len;

    while (0 < chunk.m_size && 0 == ret) {
        ret = (this->*(m_funcs[rtsp->m_input.m_rd_stat]))(rtsp, &chunk);
    }

    return ret; 
}

Int32 RtspDec::readInit(Rtsp* rtsp, AVal*) {
    RtspInput* chn = &rtsp->m_input;

    CacheCenter::zero(chn, sizeof(*chn));
    chn->m_rd_stat = ENUM_READ_HEADER;
    return 0;
}

Void RtspDec::growHead(RtspInput* chn, Int32 cap) {
    if (chn->m_txt_cap < cap) { 
        /* one more byte for '\0' */
        chn->m_txt = (Char*)realloc(chn->m_txt, cap + 1);
        chn->m_txt_cap = cap;
        chn->m_txt[cap] = BYTE_RTSP_NULL;
    }
}

Int32 RtspDec::chkStat(RtspInput* chn) {
    Int32 ret = 0;

    if (MAX_HEADER_FIELDS_NUM <= chn->m_field_num) {
        ret = ENUM_RTSP_ERR_HEADER_EXCEED_LINE;
    } else if (MAX_HTML_HEADER_SIZE <= chn->m_total) { 
        ret = ENUM_RTSP_ERR_HEADER_EXCEED_SIZE;
    } else {
        ret = 0;
    }

    return ret;
}

Int32 RtspDec::readHead(Rtsp* rtsp, AVal* chunk) {
    Bool bFoundLength = FALSE;
    Int32 ret = 0;
    Int32 used = 0;
    Span* span = NULL;
    RtspInput* chn = &rtsp->m_input;
    Char curr = '\0';
    Char last = '\0'; 
    
    /* start length of header */    
    if (0 < chn->m_total) {
        last = chn->m_txt[chn->m_total - 1];
    }
    
    for (Int32 i = 0; i < chunk->m_size; ++i) {
        ++chn->m_total;
        
        ret = chkStat(chn);
        if (0 != ret) {
            break;
        }
        
        curr = chunk->m_data[i];
        
        if (BYTE_RTSP_LF != curr) { 
            if (BYTE_RTSP_NULL != curr) {
                ++chn->m_line_size;
            } else {
                /* html header must not have '\0' */
                ret = ENUM_RTSP_ERR_HEADER_CHAR_INVAL;
                break;
            }
        } else {
            if (0 < i) {
                last = chunk->m_data[i - 1];
            } 

            if (BYTE_RTSP_CR == last) {
                /* reach end of line, skip \r */
                --chn->m_line_size;

                if (0 < chn->m_line_size) {
                    /* reach end of the line, record it */
                    span = &chn->m_fields[chn->m_field_num];
                    
                    span->m_low = chn->m_line_start;
                    span->m_high = chn->m_line_start + chn->m_line_size;

                    /* go beyond \r\n for the next line */
                    ++chn->m_field_num;
                    chn->m_line_start += chn->m_line_size + DEF_RTSP_CR_LF_SIZE; 
                    chn->m_line_size = 0; 
                } else {
                    /* reach end of head, copy head data include '\r\n\r\n */
                    chn->m_line_start += DEF_RTSP_CR_LF_SIZE;
                    
                    bFoundLength = TRUE;
                    break;
                }
            } else {
                /* ignore signle \n */
                ++chn->m_line_size;
            }
        } 
    } 

    if (0 != ret) {
        LOG_ERROR("rtsp_read_header| ret=%d| line_num=%d|"
            " curr_line_size=%d|"
            " total=%d| msg=read error|",
            ret,
            chn->m_field_num,
            chn->m_line_size,
            chn->m_total);
        
        return ret;
    }

    /* alloc max head size buffer */
    if (NULL == chn->m_txt) {
        growHead(chn, MAX_HTML_HEADER_SIZE);
    }

    used = chn->m_total - chn->m_upto; 
    CacheCenter::copy(chn->m_txt + chn->m_upto, chunk->m_data, used);

    chn->m_upto = chn->m_total;
    chunk->m_data += used;
    chunk->m_size -= used;

    if (!bFoundLength) {
        return 0;
    } else {
        ret = begBody(rtsp);
        return ret;
    }
}

Int32 RtspDec::getContentLen(RtspInput* chn) {
    Int32 ret = 0;
    Int32 len = 0;
    Char* psz = NULL;
    Span* span = NULL;   
   
    for (int i=0; i<chn->m_field_num; ++i) {
        span = &chn->m_fields[i];

        psz = chn->m_txt + span->m_low;
        
        ret = strncmp(psz, PREFIX_CONTENT_LENGTH,  
            PREFIX_CONTENT_LENGTH_LEN);
        if (0 != ret) {
            continue;
        } else {
            /* found content-length */
            psz += PREFIX_CONTENT_LENGTH_LEN;
            len = atoi(psz);

            return len;
        } 
    }

    /* if not found content-length field, then default is 0 */
    return 0;
}

Int32 RtspDec::begBody(Rtsp* rtsp) {
    Int32 ret = 0;
    RtspInput* chn = &rtsp->m_input;

    /* mark head size */
    chn->m_head_size = chn->m_total;
    chn->m_body_size = getContentLen(chn);
    
    if (0 < chn->m_body_size) {   
        chn->m_total += chn->m_body_size;

        /* alloc the whole html buffer */
        growHead(chn, chn->m_total); 

        chn->m_rd_stat = ENUM_READ_BODY;
    } else if (0 == chn->m_body_size) {        
        ret = endBody(rtsp);
    } else {
        /* if invalid content-length */
        ret = ENUM_RTSP_ERR_CONTENT_LENGTH_INVAL;
    }

    return ret;
}

Int32 RtspDec::readBody(Rtsp* rtsp, AVal* chunk) {
    Int32 ret = 0;
    Int32 len = 0;
    RtspInput* chn = &rtsp->m_input;

    len = chn->m_total - chn->m_upto;
    if (len > chunk->m_size) {
        len = chunk->m_size;
    }
    
    CacheCenter::copy(chn->m_txt + chn->m_upto, chunk->m_data, len);
    chunk->m_size -= len;
    chunk->m_data += len;
    chn->m_upto += len;

    if (chn->m_upto == chn->m_total) {
        ret = endBody(rtsp);
        return ret;
    } else {
        return 0;
    }
}

Int32 RtspDec::endBody(Rtsp* rtsp) {
    Int32 ret = 0; 
    CacheHdr* hdr = NULL;
    HtmlMsg* msg = NULL;
    RtspInput* chn = &rtsp->m_input;
    RtspNode* entity = rtsp->m_entity;   

    hdr = RtspHandler::creatRtspMsg();
    msg = MsgCenter::body<HtmlMsg>(hdr); 

    chn->m_txt[chn->m_total] = BYTE_RTSP_NULL;
    
    msg->m_txt = chn->m_txt;
    msg->m_txt_cap = chn->m_txt_cap;
    msg->m_head_size = chn->m_head_size;
    msg->m_body_size = chn->m_body_size;
    
    CacheCenter::copy(msg->m_fields, chn->m_fields, 
        sizeof(Span) * chn->m_field_num);
    msg->m_field_num = chn->m_field_num;

    /* reset param for next read */
    CacheCenter::zero(chn, sizeof(*chn));
    chn->m_rd_stat = ENUM_READ_HEADER;
    
    ret = entity->recv(entity, hdr); 
    return ret;
}
