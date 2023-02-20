#ifndef __RTPMSG_H__
#define __RTPMSG_H__
#include"sysoper.h"
#include"sock/commsock.h"
#include"encoding/prototype.h"


static const Byte DEF_RTP_VERSION = 0x2;
static const Int32 MAX_CSRC_CNT_ONCE = 16;
static const Int32 MAX_REPORT_BLOCK_CNT = 32;

struct RtpHdr {
    Byte m_version;   // version, fix with 2
    Byte m_padding;    // check if padding
    Byte m_ext;        // check if has extention
    Byte m_cc;         // count of csrc

    Byte m_marker;    // marker of the boundary of msg
    Byte m_pt;         // payload type 
    Uint16 m_seq;       // seq number incremented by one for each package
    
    Uint32 m_ts;        // monotonical timestamp of the package 
    Uint32 m_ssrc;      //
    Uint32 m_csrc[MAX_CSRC_CNT_ONCE];
};

struct RtcpHdr {
    Byte m_version;   // version, fix with 2
    Byte m_padding;    // check if padding
    Byte m_rc;         // count of reception blocks 
    Byte m_pt;         // payload type 
    Uint32 m_length;   // count of  32 bits followed
};

struct RtcpSenderInfo {
    Uint32 m_ntp_high;
    Uint32 m_ntp_low;
    Uint32 m_rtp_ts;
    Uint32 m_snd_pkt_cnt;
    Uint32 m_snd_octet_cnt;
};

struct RtcpReportBlock {
    Uint32 m_ssrc;      // ssrc of this block belongs to
    Uint32 m_frac_lost;
    Uint32 m_cumul_pkg_lost;

    Uint32 m_ext_max_seq;
    Uint32 m_jitter;
    Uint32 m_lsr_ts;
    Uint32 m_delay_lsr;
};

struct RtcpSR {
    Int32 m_length;
    Uint32 m_ssrc;      // ssrc of sender
    Int32 m_report_cnt;
    RtcpSenderInfo m_sender;
    RtcpReportBlock m_reports[MAX_REPORT_BLOCK_CNT];
};

struct RtcpRR {
    Int32 m_length;
    Uint32 m_ssrc;      // ssrc of sender
    Int32 m_report_cnt;
    RtcpReportBlock m_reports[MAX_REPORT_BLOCK_CNT];
};

struct RtcpSdesItem {
    Byte m_type;    // type == 0 indicates end of item
    AVal m_val; // octet count followed, if type ==0, then no this length
};


/** begin kindes of items ****
    * CName: 1
    * Name: 2
    * EMAIN: 3
    * PHONE: 4
    * LOC: 5
    * TOOL: 6
    * NOTE: 7
    * PRIV: 8
** end kinds of items in chunks ***/

struct RtcpBye {
    RtcpHdr m_hdr;
    Uint32 m_ssrc[0];      // ssrc defined by rc
    // followed the exits ssrc
    Byte m_reason_len;      // 8-bit count, this reason is optional part
    Byte m_reason[0]; 
};

struct RtcpApp {
    RtcpHdr m_hdr;
    Uint32 m_ssrc;      // ssrc
    Byte m_data[0];
};

struct RtpPkg {
    AddrInfo m_origin;
    Int32 m_length;
    Int32 m_payload_skip;
    Byte m_data[0];
};

struct MsgUdpSend {
    Byte* m_body;
    Int32 m_hdr_upto;
    Int32 m_hdr_size;
    Int32 m_body_upto;
    Int32 m_body_size;
    AddrInfo m_dst_addr;
    Byte m_head[0];
};

enum EnumRtcpMsgType {
    ENUM_RTCP_MSG_FIR = 192,
    ENUM_RTCP_MSG_NACK = 193,
    ENUM_RTCP_MSG_SMPTETC = 194,
    ENUM_RTCP_MSG_IJ = 195,
    
    ENUM_RTCP_MSG_SR = 200,
    ENUM_RTCP_MSG_RR = 201,
    ENUM_RTCP_MSG_SDES = 202,
    ENUM_RTCP_MSG_BYE = 203,
    ENUM_RTCP_MSG_APP = 204,
    ENUM_RTCP_MSG_TOKEN = 210,
};


#endif

