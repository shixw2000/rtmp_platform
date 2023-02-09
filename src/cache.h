#ifndef __CACHE_H__
#define __CACHE_H__
#include"sysoper.h"
#include"listnode.h"


struct CacheHdr;
struct Cache;
struct RtmpPkg;

class CacheCenter {
public: 
    static CacheHdr* creat(Int32 type, Int32 buff_size);
    static void free(CacheHdr* hdr); 

    static Int32 getType(CacheHdr* hdr); 
    
    static Byte* buffer(CacheHdr* hdr);
    
    static void queue(CacheHdr* hdr, list_head* list); 
    static CacheHdr* cast(list_node* node); 

    /* these two functions just set the cache, not refereneced */
    static Cache* getCache(CacheHdr* hdr);
    static Void setCache(CacheHdr* hdr, Cache* cache);
    
    static Cache* alloc(Int32 capacity);
    static void del(Cache* cache); 
    
    static Cache* ref(Cache* cache); 
    static Byte* data(Cache* cache);

    static Void zero(Void* data, Int32 len);
    static Void copy(Void* dst, const Void* src, Int32 len);
};


class MsgCenter {
public: 
    template<typename T>
    static CacheHdr* creatMsg(Int32 type, Int32 extLen = 0) {
        Int32 size = sizeof(T) + extLen;
        CacheHdr* hdr = NULL;

        hdr = CacheCenter::creat(type, size); 
        if (NULL != hdr) {
            CacheCenter::zero(CacheCenter::buffer(hdr), sizeof(T));
        }
        
        return hdr;
    }
    
    template<typename T>
    static T* body(CacheHdr* hdr) {
        T* p = (T*)CacheCenter::buffer(hdr);

        return p;
    } 

    template<typename T>
    static Cache* creatCache(Int32 extLen) {
        Int32 total = sizeof(T) + extLen;
        Cache* cache = NULL;

        cache = CacheCenter::alloc(total);
        if (NULL != cache) {
            CacheCenter::zero(CacheCenter::data(cache), sizeof(T));
        }
        
        return cache;
    }

    template<typename T>
    static T* cast(Cache* cache) {
        T* p = (T*)CacheCenter::data(cache);

        return p;
    } 

    static CacheHdr* creatExchMsg(Int32 type, Int64 val);

    static Int64 getExchMsgVal(CacheHdr* hdr);
    
    static Bool chkSysMsg(CacheHdr* hdr); 

    /* copy data1, and refereces data2 with cache */
    static CacheHdr* creatSendMsg(Byte* data1, Int32 data1Len,
        Byte* data2, Int32 data2Len, Cache* cache);

    /* copy data1 and data2 both */
    static CacheHdr* creat2SendMsg(Byte* data1, Int32 data1Len,
        Byte* data2, Int32 data2Len);
};

#endif

