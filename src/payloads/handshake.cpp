#include<openssl/hmac.h>
#include<openssl/evp.h>
#include"handshake.h"
#include"analyser.h"


static const Byte GenuineFMSKey[] = {
    0x47, 0x65, 0x6e, 0x75, 0x69, 0x6e, 0x65, 0x20, 
    0x41, 0x64, 0x6f, 0x62, 0x65, 0x20, 0x46, 0x6c,
    0x61, 0x73, 0x68, 0x20, 0x4d, 0x65, 0x64, 0x69,
    0x61, 0x20, 0x53, 0x65, 0x72, 0x76, 0x65, 0x72,
    0x20, 0x30, 0x30, 0x31,	/* Genuine Adobe Flash Media Server 001 */

    0xf0, 0xee, 0xc2, 0x4a, 0x80, 0x68, 0xbe, 0xe8, 
    0x2e, 0x00, 0xd0, 0xd1, 0x02, 0x9e, 0x7e, 0x57, 
    0x6e, 0xec, 0x5d, 0x2d, 0x29, 0x80, 0x6f, 0xab,
    0x93, 0xb8, 0xe6, 0x36, 0xcf, 0xeb, 0x31, 0xae
};				/* 68 */

static const Byte GenuineFPKey[] = {
    0x47, 0x65, 0x6E, 0x75, 0x69, 0x6E, 0x65, 0x20,
    0x41, 0x64, 0x6F, 0x62, 0x65, 0x20, 0x46, 0x6C,
    0x61, 0x73, 0x68, 0x20, 0x50, 0x6C, 0x61, 0x79, 
    0x65, 0x72, 0x20, 0x30, 0x30, 0x31,	/* Genuine Adobe Flash Player 001 */
  
    0xF0, 0xEE, 0xC2, 0x4A, 0x80, 0x68, 0xBE, 0xE8,
    0x2E, 0x00, 0xD0, 0xD1, 0x02, 0x9E, 0x7E, 0x57,
    0x6E, 0xEC, 0x5D, 0x2D, 0x29, 0x80, 0x6F, 0xAB, 
    0x93, 0xB8, 0xE6, 0x36, 0xCF, 0xEB, 0x31, 0xAE
};				/* 62 */

Void HMACsha256(const Void* key, Int32 keyLen, 
    const Byte* data1, Int32 data1Len, 
    const Byte* data2, Int32 data2Len,
    Byte* md) {
    const EVP_MD* evp = EVP_sha256();
    HMAC_CTX *ctx = NULL;
    
    ctx = HMAC_CTX_new();
    if (NULL != ctx) {
        HMAC_Init_ex(ctx, key, keyLen, evp, NULL);

        HMAC_Update(ctx, data1, data1Len);
        if (NULL != data2 && 0 < data2Len) {
            HMAC_Update(ctx, data2, data2Len);
        }

        HMAC_Final(ctx, md, NULL);

        HMAC_CTX_free(ctx);
    }
}

const HandShake::getoff HandShake::m_getdig[2] = {
    &HandShake::getDigestOff_1,

    &HandShake::getDigestOff_2
};

Void HandShake::digestSig(const Void* key, Int32 keyLen,
    const Byte* buff, Int32 total, Int32 offset, Byte* ptr) {
    Int32 len2 = 0;
    const Byte* d2 = NULL;

    len2 = total - offset - DEF_DIGEST_LENGTH;
    d2 = &buff[offset + DEF_DIGEST_LENGTH];

    HMACsha256(key, keyLen, buff, offset, d2, len2, ptr);
}

Void HandShake::digestMac(const Void* key, Int32 keyLen,
    const Byte dig[], const Byte buff[], Int32 size, 
    Byte* ptr) {
    Byte sig[DEF_DIGEST_LENGTH] = {0};
    
    HMACsha256(key, keyLen, dig, DEF_DIGEST_LENGTH, NULL, 0, sig); 
    HMACsha256(sig, DEF_DIGEST_LENGTH, buff, size, NULL, 0, ptr);
}

Int32 HandShake::getDigestOff_1(const Byte buff[]) {
    Int32 offset = 0;
    const Byte* ptr = &buff[8];

    for (int i=0; i<4; ++i) {
        offset += ptr[i];
    }

    /* part one before digest */
    offset = (offset % 728) + 12;
    return offset;
}

Int32 HandShake::getDigestOff_2(const Byte buff[]) {
    Int32 offset = 0;
    const Byte* ptr = &buff[772];

    for (int i=0; i<4; ++i) {
        offset += ptr[i];
    }

    /* part one before digest */
    offset = (offset % 728) + 776;
    return offset;
}

Void HandShake::creatReq(const Void* key, Int32 keyLen, 
    Byte buff[], RtmpHandshake* handshake) {
    Uint32 val = 0;
    Int32 offset = 0;
    Chunk chunk = {buff, 8};
    Builder builder(&chunk);

    /* timer */
    val = getMonoTimeMs();
    builder.build32(&val);
    
    /* version */
    val = 0x20221216;
    builder.build32(&val);

    /* set random */
    getRand(&buff[8], RTMP_SIG_SIZE - 8);

    offset = getDigestOff_1(buff); 
    digestSig(key, keyLen, buff, RTMP_SIG_SIZE, offset, 
        handshake->m_localDigest);

    /* copy to buffer */
    memcpy(&buff[offset], handshake->m_localDigest, DEF_DIGEST_LENGTH);
}

Int32 HandShake::verifyReq(const Void* key, Int32 keyLen, 
    const Byte buff[]) {
    Int32 offset = 0;
    Byte sig[DEF_DIGEST_LENGTH] = {0};

    for (int i=0; i<2; ++i) {
        offset = (m_getdig[i])(buff); 
        
        digestSig(key, keyLen, buff, RTMP_SIG_SIZE, offset, sig);
        if (0 == memcmp(sig, &buff[offset], DEF_DIGEST_LENGTH)) {
            return offset;
        } 
    }

    return -1;
}

Void HandShake::creatS0S1(Byte buff[], RtmpHandshake* handshake) {
    buff[0] = RTMP_DEF_HANDSHAKE_VER;
    
    creatReq(GenuineFMSKey, 36, &buff[1], handshake);
}

Void HandShake::creatC0C1(Byte buff[], RtmpHandshake* handshake) {
    buff[0] = RTMP_DEF_HANDSHAKE_VER;
    
    creatReq(GenuineFPKey, 30, &buff[1], handshake);
}

Int32 HandShake::parseReq(const Void* key, Int32 keyLen, 
    Byte buff[], RtmpHandshake* handshake) {
    Int32 offset = 0;
    Chunk chunk = {buff, 8};
    Parser parser(&chunk);

    offset = verifyReq(key, keyLen, buff);
    if (0 <= offset) {
        memcpy(handshake->m_peerDigest, &buff[offset], DEF_DIGEST_LENGTH); 

        parser.parse32(&handshake->m_peerTs);
        parser.parse32(&handshake->m_peerMark);
        return 0;
    } else {
        handshake->m_peerTs = 0;
        handshake->m_peerMark = 0;
        LOG_WARN("parse_handshake_req| msg=verify req error, but ignore|");
        
        return 0;
    }
}

Int32 HandShake::parseC0C1(Byte buff[], RtmpHandshake* handshake) {
    Int32 ret = 0;

    if (RTMP_DEF_HANDSHAKE_VER == buff[0]) {
        ret = parseReq(GenuineFPKey, 30, &buff[1], handshake); 
    } else {
        ret = -1;
    }
    
    return ret;
}

Int32 HandShake::parseS0S1(Byte buff[], RtmpHandshake* handshake) {
    Int32 ret = 0;

    if (RTMP_DEF_HANDSHAKE_VER == buff[0]) {
        ret = parseReq(GenuineFMSKey, 36, &buff[1], handshake); 
    } else {
        ret = -1;
    }
    
    return ret;
}

Void HandShake::creatS2(Byte buff[], RtmpHandshake* handshake) {    
    creatRsp(GenuineFMSKey, sizeof(GenuineFMSKey), 
        handshake->m_peerTs, handshake->m_peerDigest, buff);
}

Void HandShake::creatC2(Byte buff[], RtmpHandshake* handshake) { 
    creatRsp(GenuineFPKey, sizeof(GenuineFPKey), 
        handshake->m_peerTs, handshake->m_peerDigest, buff);
}

Void HandShake::creatRsp(const Void* key, Int32 keyLen,
    Uint32 peerTs, const Byte dig[], Byte buff[]) {
    Int32 len = RTMP_SIG_SIZE - DEF_DIGEST_LENGTH;
    Uint32 val = 0;
    Chunk chunk = {buff, 8};
    Builder builder(&chunk);

    /* timer */
    val = getMonoTimeMs();
    builder.build32(&val);
    
    /* version */
    builder.build32(&peerTs);

    /* set random */
    getRand(&buff[8], RTMP_SIG_SIZE - 8);

    digestMac(key, keyLen, dig, buff, len, &buff[len]); 
}

Int32 HandShake::verifyRsp(const Void* key, Int32 keyLen,
    const Byte dig[], Byte buff[]) {
    Int32 len = RTMP_SIG_SIZE - DEF_DIGEST_LENGTH;
    Byte sig[DEF_DIGEST_LENGTH] = {0};
    
    digestMac(key, keyLen, dig, buff, len, sig);
    if (0 == memcmp(&buff[len], sig, DEF_DIGEST_LENGTH)) {
        return 0;
    } else {
        return -1;
    }
}

Bool HandShake::verifySimple(const Byte dig[], Byte buff[]) {
    Int32 offset = 0;

    offset = getDigestOff_1(buff); 
    if (0 == memcmp(dig, &buff[offset], DEF_DIGEST_LENGTH)) {
        return TRUE;
    } else { 
        return FALSE;
    }
}

Int32 HandShake::parseC2(Byte buff[], RtmpHandshake* handshake) {
    Int32 ret = 0;
    Bool bOk = FALSE;

    bOk = verifySimple(handshake->m_localDigest, buff);
    if (bOk) {
        return 0;
    } else { 
        ret = verifyRsp(GenuineFPKey, sizeof(GenuineFPKey), 
            handshake->m_localDigest, buff);

        return ret;
    }
}

Int32 HandShake::parseS2(Byte buff[], RtmpHandshake* handshake) {
    Int32 ret = 0;

    ret = verifyRsp(GenuineFMSKey, sizeof(GenuineFMSKey), 
        handshake->m_localDigest, buff);
    return ret;
}

