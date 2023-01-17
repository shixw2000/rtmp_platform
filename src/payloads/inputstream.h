#ifndef __INPUTSTREAM_H__
#define __INPUTSTREAM_H__
#include"globaltype.h"
#include"datatype.h"


struct RtmpNode;
struct Rtmp;
struct Chunk; 

class RtmpInput {
    typedef Int32 (RtmpInput::*PFunc)(Rtmp* rtmp, Chunk* chunk);
    
public:    
    Int32 parseMsg(Rtmp* rtmp, Byte* data, Int32 len);

private: 
    Void resetChnCache(RtmpChn* chn);
    Void resetChnPkg(RtmpChn* chn);
    Bool fill(RtmpChn* chn, Chunk* src);
    
    Int32 readInit(Rtmp* rtmp, Chunk* chunk);
    Int32 readC0C1(Rtmp* rtmp, Chunk* chunk);
    Int32 readC2(Rtmp* rtmp, Chunk* chunk); 
    
    Int32 readBasic(Rtmp* rtmp, Chunk* chunk);
    Int32 readHeader(Rtmp* rtmp, Chunk* chunk);
    Void parseHdr(Rtmp* rtmp);
    Int32 readExtTime(Rtmp* rtmp, Chunk* chunk);
    Void parseExtTime(Rtmp* rtmp);
    Int32 begChunk(Rtmp* rtmp);
    Int32 readChunk(Rtmp* rtmp, Chunk* chunk);
    Int32 endChunk(Rtmp* rtmp);
    
private:
    static const PFunc m_funcs[ENUM_RTMP_RD_END];
};


#endif

