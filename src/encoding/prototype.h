#ifndef __PROTOTYPE_H__
#define __PROTOTYPE_H__
#include"sysoper.h"


enum EnumEncType {
    ENUM_ENC_NULL = 0,
        
    U_INT_1 = 1,
    U_INT_2,
    U_INT_4,
    U_INT_5,
    U_INT_6,
    U_INT_7,
    U_INT_8,
    U_INT_16,
    U_INT_24,
    U_INT_32,
    CHUNK_8, 

    ENUM_ENC_END
};


struct EncodingRule {
    Int32 m_enc_type;
    Int32 m_offset;
};

struct AVal {
    Byte* m_data;
    Int32 m_size;
};

#endif

