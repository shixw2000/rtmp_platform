#ifndef __SYSOPER_H__
#define __SYSOPER_H__
#include<stdio.h>
#include<errno.h>
#include<string.h>


typedef unsigned char Byte;
typedef char Char;
typedef short Int16;
typedef unsigned short Uint16;
typedef int Int32;
typedef unsigned int Uint32;
typedef long long Int64;
typedef unsigned long long Uint64;
typedef bool Bool;
typedef void Void;

enum BOOL_VAL {
    FALSE = 0,
    TRUE = 1
};

#ifndef NULL
#define NULL 0
#endif


#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN
#endif

#define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))
#define CAS(ptr,test,val) __sync_bool_compare_and_swap((ptr), (test), (val))
#define CMPXCHG(ptr,test,val) __sync_val_compare_and_swap((ptr), (test), (val)) 
#define ATOMIC_SET(ptr,val) __sync_lock_test_and_set((ptr), (val))
#define ATOMIC_CLEAR(ptr) __sync_lock_release(ptr)
#define ATOMIC_FETCH_INC(ptr) __sync_fetch_and_add((ptr), 1)
#define ATOMIC_INC(ptr) __sync_add_and_fetch((ptr), 1)
#define ATOMIC_DEC(ptr) __sync_sub_and_fetch((ptr), 1)

#define offset_of(TYPE, MEMBER) ((long) &((TYPE *)0)->MEMBER)
#define containof(ptr, type, member) ({			\
	const typeof(((type *)0)->member)* __mptr = (ptr);	\
	(type *)((char*)__mptr - offset_of(type, member)); })

#define count_of(arr) (sizeof(arr)/sizeof((arr)[0]))

#define I_FREE(x) do { if (NULL != (x)) {delete (x); (x)=NULL;} } while (0)
#define I_NEW(type, x)  ((x) = new type)
#define I_NEW_P(type, x, v1, ...)  ((x) = new type(v1, ##__VA_ARGS__))
#define ARR_FREE(x) do { if (NULL != (x)) {delete[] (x); (x)=NULL;} } while (0)
#define ARR_NEW(type,size, x) ((x) = new type[size])

#define ERR_MSG() strerror(errno)


#endif

