#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__
#include"sysoper.h"
#include"prototype.h"


class ProtoDecoder {
protected:
    ProtoDecoder();

public:
    Void reset(Void* data, Int32 size);
    
    Int32 parseChunk(Int32 ruleNum, Int32 length, AVal* chunk);
    
    Int32 parseChunk_8(Int32 ruleNum, AVal* chunk);

    Int32 parseRule(Int32 ruleNum, Int32 enc_type, Void* obj);
  
    template<Int32 x>
    Int32 parseBit(Int32 ruleNum, Byte* out);

    template<int x>
    Int32 parseByte(Int32 ruleNum, Uint32* out);

protected:
    Byte* m_data;
    Int32 m_size;
    Int32 m_upto;
    Int32 m_bit_pos;
};

#endif

