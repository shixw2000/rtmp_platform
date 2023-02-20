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

struct Token {
    Char* m_token;
    Int32 m_size;
};

struct AVal {
    Byte* m_data;
    Int32 m_size;
};

static const Int32 MAX_IP_SIZE = 128;
static const Int32 MAX_PATH_LEN = 256;
static const Int32 MAX_NAME_LEN = 128;
static const Int32 MAX_SESS_ID_LEN = 64;
static const Int32 MAX_URL_LEN = 256;
static const Int32 MAX_ADDR_SIZE = 128;
static const Int32 COMM_BUFFER_SIZE = 128;
static const Char DEF_URL_SEP_CHAR = '/';
static const Char DEF_URL_EQUATION_CHAR = '=';
static const Char DEF_URL_RANGE_CHAR = '-';
static const Char DEF_QUESTION_MARK_CHAR = '?';
static const Char DEF_COLON_CHAR = ':';
static const Char DEF_COMMA_CHAR = ',';
static const Char DEF_SEMICOLON_CHAR = ';';
static const Char DEF_AT_CHAR = '@';
static const Char DEF_SQUARE_BRACKET_LEFT = '[';
static const Char DEF_SQUARE_BRACKET_RIGHT = ']';
static const Char DEF_CR_CHAR = '\r';
static const Char DEF_LF_CHAR = '\n';
static const Char DEF_NULL_CHAR = '\0'; 

static const Char DEF_SCHEMA_MARKER_STR[] = "://";
static const Int32 DEF_SCHEMA_MARKER_STR_SIZE = 
    sizeof(DEF_SCHEMA_MARKER_STR) - 1;

static const Char DEF_BLANK_STR[] = " \t";
static const Int32 DEF_BLANK_STR_SIZE = sizeof(DEF_BLANK_STR) - 1;
static const Char DEF_NEWLINE_STR[] = "\r\n";
static const Char DEF_LWS_STR[] = " \t\r\n";
static const Int32 DEF_LWS_STR_SIZE = sizeof(DEF_LWS_STR) - 1;

struct IpInfo {
    Int32 m_port;
    Char m_ip[MAX_IP_SIZE];
};

struct AddrInfo {
    Int32 m_size;
    Byte m_addr[MAX_ADDR_SIZE];
};

static const Token DEF_EMPTY_TOKEN = { NULL, 0 };

static const AVal DEF_EMPTY_AVAL = { NULL, 0 };


#endif

