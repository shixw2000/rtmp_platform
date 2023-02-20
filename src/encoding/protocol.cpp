#include<stdlib.h>
#include"protocol.h"
#include"misc.h"


template<Int32 x>
Int32 ProtoDecoder::parseBit(Int32 ruleNum, Byte* out) {
    Int32 mask = (0x1 << x) - 1;
    Int32 shift = 8 - x - m_bit_pos;

    if (m_upto < m_size && 0 <= shift) {
        if (NULL != out) {
            *out = (m_data[m_upto] >> shift) & mask; 
        }

        if (0 < shift) {
            m_bit_pos += x;
        } else {
            m_bit_pos = 0;
            ++m_upto;
        }

        return 0;
    } else {
        LOG_ERROR("parse_bit| rule_num=%d| upto=%d| size=%d| bit_pos=%d|"
            " bit_x=%d| msg=invalid param|",
            ruleNum, m_upto, m_size, m_bit_pos, x);
        
        return -1;
    } 
}

template Int32 ProtoDecoder::parseBit<1>(Int32 ruleNum, Byte* out);
template Int32 ProtoDecoder::parseBit<2>(Int32 ruleNum, Byte* out);
template Int32 ProtoDecoder::parseBit<3>(Int32 ruleNum, Byte* out);
template Int32 ProtoDecoder::parseBit<4>(Int32 ruleNum, Byte* out);
template Int32 ProtoDecoder::parseBit<5>(Int32 ruleNum, Byte* out);
template Int32 ProtoDecoder::parseBit<6>(Int32 ruleNum, Byte* out);
template Int32 ProtoDecoder::parseBit<7>(Int32 ruleNum, Byte* out);

template<int x>
Int32 ProtoDecoder::parseByte(Int32 ruleNum, Uint32* out) {
    Uint32 n = 0;

    if (m_upto + x <= m_size && 0 == m_bit_pos) {
        if (NULL != out) {
            for (int i=0; i<x; ++i) {
                n <<= 8;
                n += m_data[m_upto+i];
            }

            *out = n;
        }

        m_upto += x;
        return 0;
    } else {
        LOG_ERROR("parse_byte| rule_num=%d| upto=%d| size=%d|"
            " bit_pos=%d| byte_x=%d|  msg=invalid param|",
            ruleNum, m_upto, m_size, m_bit_pos, x);
        
        return -1;
    }
}

template Int32 ProtoDecoder::parseByte<1>(Int32 ruleNum, Uint32* out);
template Int32 ProtoDecoder::parseByte<2>(Int32 ruleNum, Uint32* out);
template Int32 ProtoDecoder::parseByte<3>(Int32 ruleNum, Uint32* out);
template Int32 ProtoDecoder::parseByte<4>(Int32 ruleNum, Uint32* out);


ProtoDecoder::ProtoDecoder() {
    m_data = NULL;
    m_size = 0;
    m_upto = 0;
    m_bit_pos = 0;
}

Void ProtoDecoder::reset(Void* data, Int32 size) {
    m_data = (Byte*)data;
    m_size = size;
    m_upto = 0;
    m_bit_pos = 0;
}

Int32 ProtoDecoder::parseChunk(Int32 ruleNum, Int32 length, AVal* chunk) {    
    if (m_upto + length <= m_size && 0 == m_bit_pos) {
        if (NULL != chunk) {
            chunk->m_size = length;
            chunk->m_data = &m_data[m_upto];
        }

        m_upto += length;
        return 0;
    } else {
        LOG_ERROR("parse_chunk| rule_num=%d| upto=%d| size=%d| bit_pos=%d|"
            " length=%d| msg=invalid param|",
            ruleNum, m_upto, m_size, m_bit_pos, length);
        
        return -1;
    } 
}

Int32 ProtoDecoder::parseChunk_8(Int32 ruleNum, AVal* chunk) {
    Int32 ret = 0;
    Int32 length = 0;

    do {
        ret = parseRule(ruleNum, U_INT_8, &length);
        if (0 != ret) {
            break;
        }

        ret = parseChunk(ruleNum, length, chunk);
        if (0 != ret) {
            break;
        }
    } while (0);

    LOG_ERROR("parse_chunk_8| rule_num=%d| upto=%d| size=%d| bit_pos=%d|"
        " length=%d| msg=invalid param|",
        ruleNum, m_upto, m_size, m_bit_pos, length);
    
    return ret;
}

Int32 ProtoDecoder::parseRule(Int32 ruleNum, Int32 enc_type, Void* obj) {
    Int32 ret = 0;
    Byte* pb = (Byte*)obj;
    
    switch (enc_type) {
    case U_INT_1:
        ret = parseBit<1>(ruleNum, pb);
        break;

    case U_INT_2:
        ret = parseBit<2>(ruleNum, pb);
        break;

    case U_INT_4:
        ret = parseBit<4>(ruleNum, pb);
        break;

    case U_INT_5:
        ret = parseBit<5>(ruleNum, pb);
        break;

    case U_INT_6:
        ret = parseBit<6>(ruleNum, pb);
        break;

    case U_INT_7:
        ret = parseBit<7>(ruleNum, pb);
        break;

    case U_INT_8:
        ret = parseByte<1>(ruleNum, (Uint32*)pb);
        break;

    case U_INT_16:
        ret = parseByte<2>(ruleNum, (Uint32*)pb);
        break;

    case U_INT_24:
        ret = parseByte<3>(ruleNum, (Uint32*)pb);
        break;

    case U_INT_32:
        ret = parseByte<4>(ruleNum, (Uint32*)pb);
        break;

    case CHUNK:
        ret = parseChunk_8(ruleNum, (AVal*)pb);
        break;
        
    case CHUNK_8:
        ret = parseChunk_8(ruleNum, (AVal*)pb);
        break; 

    default: 
        LOG_ERROR("parse_obj| rule_num=%d| upto=%d|"
            " size=%d| bit_pos=%d|"
            " enc_type=%d| msg=unknown enc_type|",
            ruleNum, m_upto, m_size, m_bit_pos,
            enc_type);

        ret = -1;
        break;
    }
    
    return ret;    
}

