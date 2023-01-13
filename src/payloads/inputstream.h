#ifndef __INPUTSTREAM_H__
#define __INPUTSTREAM_H__
#include"globaltype.h"
#include"datatype.h"


struct RtmpNode;
struct Rtmp;
struct Chunk; 

class RtmpInput {
    typedef Int32 (RtmpInput::*PFunc)(RtmpNode* node, Rtmp* rtmp, Chunk* chunk);
    
public:    
    Int32 parseMsg(RtmpNode* node, Rtmp* rtmp, Byte* data, Int32 len);

private: 
    Void resetChnCache(RtmpChn* chn);
    Void resetChnPkg(RtmpChn* chn);
    Bool fill(RtmpChn* chn, Chunk* src);
    
    Int32 readInit(RtmpNode* node, Rtmp* rtmp, Chunk* chunk);
    Int32 readC0C1(RtmpNode* node, Rtmp* rtmp, Chunk* chunk);
    Int32 readC2(RtmpNode* node, Rtmp* rtmp, Chunk* chunk); 
    
    Int32 readBasic(RtmpNode* node, Rtmp* rtmp, Chunk* chunk);
    Int32 readHeader(RtmpNode* node, Rtmp* rtmp, Chunk* chunk);
    Void parseHdr(Rtmp* rtmp);
    Int32 readExtTime(RtmpNode* node, Rtmp* rtmp, Chunk* chunk);
    Void parseExtTime(Rtmp* rtmp);
    Int32 begChunk(RtmpNode* node, Rtmp* rtmp);
    Int32 readChunk(RtmpNode* node, Rtmp* rtmp, Chunk* chunk);
    Int32 endChunk(RtmpNode* node, Rtmp* rtmp);
    
private:
    static const PFunc m_funcs[ENUM_RTMP_RD_END];
};


#endif

