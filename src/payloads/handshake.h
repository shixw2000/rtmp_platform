#ifndef __HANDSHAKE_H__
#define __HANDSHAKE_H__
#include"sysoper.h"
#include"datatype.h"


class HandShake {
    typedef Int32 (*getoff)(const Byte[]);
    
public:
    Void creatS0S1(Byte buff[], RtmpHandshake* handshake);
    Void creatC0C1(Byte buff[], RtmpHandshake* handshake);

    Int32 parseS0S1(Byte buff[], RtmpHandshake* handshake);
    Int32 parseC0C1(Byte buff[], RtmpHandshake* handshake);
    
    Void creatS2(Byte buff[], RtmpHandshake* handshake);
    Void creatC2(Byte buff[], RtmpHandshake* handshake);

    Int32 parseS2(Byte buff[], RtmpHandshake* handshake);
    Int32 parseC2(Byte buff[], RtmpHandshake* handshake);

private: 
    Void digestSig(const Void* key, Int32 keyLen,
        const Byte* buff, Int32 total, 
        Int32 offset, Byte* ptr);
    
    Void digestMac(const Void* key, Int32 keyLen,
        const Byte dig[], const Byte buff[], Int32 size, 
        Byte* ptr);
    
    Void creatReq(const Void* key, Int32 keyLen, 
        Byte buff[], RtmpHandshake* handshake);
    
    Int32 verifyReq(const Void* key, Int32 keyLen, const Byte buff[]);
    
    Int32 parseReq(const Void* key, Int32 keyLen, 
        Byte buff[], RtmpHandshake* handshake);
        
    Void creatRsp(const Void* key, Int32 keyLen,
        Uint32 peerTs, const Byte dig[], Byte buff[]);
    
    Int32 verifyRsp(const Void* key, Int32 keyLen,
        const Byte dig[], Byte buff[]);

    Bool verifySimple(const Byte dig[], Byte buff[]);
    
    static Int32 getDigestOff_1(const Byte buff[]);
    static Int32 getDigestOff_2(const Byte buff[]);

private: 
    static const getoff m_getdig[2];    
};

extern Void HMACsha256(const Void* key, Int32 keyLen, 
    const Byte* data1, Int32 data1Len, 
    const Byte* data2, Int32 data2Len,
    Byte* md);

#endif

