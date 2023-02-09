#ifndef __RTSPDEC_H__
#define __RTSPDEC_H__
#include"rtspmsg.h"


struct CacheHdr;

class RtspDec {
    typedef Int32 (RtspDec::*PFunc)(Rtsp* rtsp, AVal* chunk);
    
public:    
    Int32 parseMsg(Rtsp* rtsp, Byte* data, Int32 len);

private:     
    Int32 readInit(Rtsp* rtsp, AVal* chunk);
    Int32 readHead(Rtsp* rtsp, AVal* chunk);
    Int32 readBody(Rtsp* rtsp, AVal* chunk);
    
    Void growHead(RtspInput* chn, Int32 cap);
    Int32 getContentLen(RtspInput* chn);
    Int32 begBody(Rtsp* rtsp);
    Int32 endBody(Rtsp* rtsp); 

    Int32 chkStat(RtspInput* chn);
    
private:
    static const PFunc m_funcs[ENUM_RTSP_DEC_END];
};

#endif

