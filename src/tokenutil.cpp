#include<stdlib.h>
#include"tokenutil.h"


TokenUtil::TokenUtil() : m_start(NULL), m_end(NULL) {
}

Void TokenUtil::reset(const Token* token) {
    m_start = token->m_token;
    m_end = m_start + token->m_size;
}

Void TokenUtil::reset(const Char* data, Int32 size) {
    m_start = data;
    m_end = m_start + size;
}

Void TokenUtil::reset(const Char* data) {
    m_start = data;
    m_end = m_start + (Int32)strlen(data);
} 

Void TokenUtil::setToken(Token* token, const Char* start, Int32 size) {
    if (0 > size) {
        size = (Int32)(strlen(start));
    }
    
    if (NULL != token) {
        token->m_token = (Char*)start;
        token->m_size = size;
    }
}

Void TokenUtil::getToken(const Token* token, Char* buf, Int32 max) {
    if (NULL != token && max > token->m_size) {
        memcpy(buf, token->m_token, token->m_size);
        
        buf[token->m_size] = DEF_NULL_CHAR;
    } else {
        buf[0] = DEF_NULL_CHAR;
    }
}

Bool TokenUtil::isEol() const {
    return !(m_start < m_end);
}

Void TokenUtil::setEol() {
    m_start = m_end;
}

Void TokenUtil::stripHead() {
    skipHead(DEF_LWS_STR);
}

Void TokenUtil::stripTail() {
    skipTail(DEF_LWS_STR);
}

Void TokenUtil::stripAll() {
    skipHead(DEF_LWS_STR);
    skipTail(DEF_LWS_STR);
}

Bool TokenUtil::findFirstNotOf(const Char* reject, Token* entity) const {
    Int32 len = 0;
    Bool bFound = FALSE;
    const Char* pBeg = NULL;

    len = strlen(reject);
    if (0 < len) {
        pBeg = m_start;
        
        while (pBeg < m_end) {
            bFound = isFall(reject, len, *pBeg);
            if (bFound) {
                ++pBeg;
            } else {
                setToken(entity, (Char*)pBeg, 1);
                
                return TRUE;
            }
        }
    }

    setToken(entity, NULL, 0);
    return FALSE;
}

Bool TokenUtil::findLastNotOf(const Char* reject, Token* entity) const {
    Int32 len = 0;
    Bool bFound = FALSE;
    const Char* pLast = NULL;

    len = strlen(reject);
    if (0 < len) {
        pLast = m_end - 1;
        
        while (pLast >= m_start) {
            bFound = isFall(reject, len, *pLast);
            if (bFound) {
                --pLast;
            } else {
                setToken(entity, (Char*)pLast, 1);
                
                return TRUE;
            }
        }
    }

    setToken(entity, NULL, 0);
    return FALSE;
}

Bool TokenUtil::findLastOf(const Char* accept, Token* entity) const {
    Int32 len = 0;
    Bool bFound = FALSE;
    const Char* pLast = NULL;

    len = strlen(accept);
    if (0 < len) {
        pLast = m_end - 1;
        
        while (pLast >= m_start) {
            bFound = isFall(accept, len, *pLast);
            if (!bFound) {
                --pLast;
            } else {
                setToken(entity, (Char*)pLast, 1);
                
                return TRUE;
            }
        }
    }

    setToken(entity, NULL, 0);
    return FALSE;
}

Bool TokenUtil::findFirstOf(const Char* accept, Token* entity) const {
    Int32 len = 0;
    Bool bFound = FALSE;
    const Char* pBeg = NULL;

    len = strlen(accept);
    if (0 < len) {
        pBeg = m_start;
        
        while (pBeg < m_end) {
            bFound = isFall(accept, len, *pBeg);
            if (!bFound) {
                ++pBeg;
            } else {
                setToken(entity, (Char*)pBeg, 1);
                
                return TRUE;
            }
        }
    }

    setToken(entity, NULL, 0);
    return FALSE;
}

Bool TokenUtil::findChar(Char ch, Token* entity) const {
    const Char* pBeg = m_start;

    while (pBeg < m_end) {
        if (ch != *pBeg) {
            ++pBeg;
        } else {
            setToken(entity, (Char*)pBeg, 1);

            return TRUE;
        }
    }

    setToken(entity, NULL, 0);
    return FALSE;
}

Bool TokenUtil::findStr(const Char* str, Token* entity) const { 
    Int32 len = 0;
    const Char* pBeg = NULL;
    const Char* pEnd = NULL;

    len = strlen(str);
    if (0 < len) { 
        pBeg = m_start;
        pEnd = m_end - len;
        while (pBeg <= pEnd) {
            if (0 != strncmp(pBeg, str, len)) {
                ++pBeg;
            } else {
                setToken(entity, (Char*)pBeg, len);

                return TRUE;
            }
        }
    }

    setToken(entity, NULL, 0);
    return FALSE;
}

Bool TokenUtil::startWith(const Char* str, Token* entity) const { 
    Int32 len = 0;

    len = strlen(str);
    if (0 < len) {
        if (0 == strncmp(m_start, str, len)) {
            setToken(entity, (Char*)m_start, len);

            return TRUE;
        }
    }

    setToken(entity, NULL, 0);
    return FALSE;
}

Bool TokenUtil::startCaseWith(const Char* str, Token* entity) const { 
    Int32 len = 0;

    len = strlen(str);
    if (0 < len) {
        if (0 == strncasecmp(m_start, str, len)) {
            setToken(entity, (Char*)m_start, len);

            return TRUE;
        }
    }

    setToken(entity, NULL, 0);
    return FALSE;
}

Bool TokenUtil::startEqual(Char ch) const { 
    if (!isEol()) {
        if (ch == *m_start) {
            return TRUE;
        }
    }

    return FALSE;
}

Void TokenUtil::cutBefore(const Token* marker, Token* entity) const {
    Int32 len = 0;
    
    if (m_start < marker->m_token) {
        len = (Int32)(marker->m_token - m_start);
        
        setToken(entity, (Char*)m_start, len);
    } else {
        setToken(entity, NULL, 0);
    }
}

Void TokenUtil::cutAfter(const Token* marker, Token* entity) const {
    Int32 len = 0;
    const Char* psz = marker->m_token + marker->m_size;
    
    if (m_end > psz) {
        len = (Int32)(m_end - psz);
        
        setToken(entity, (Char*)psz, len);
    } else {
        setToken(entity, NULL, 0);
    }
}

Void TokenUtil::cutFrom(const Token* marker, Token* entity) const {
    Int32 len = 0;
    
    if (m_end > marker->m_token) {
        len = (Int32)(m_end - marker->m_token);
        
        setToken(entity, marker->m_token, len);
    } else {
        setToken(entity, NULL, 0);
    }
}

Void TokenUtil::copy(Token* entity) const {
    Int32 len = 0;

    len = (Int32)(m_end - m_start);
    setToken(entity, (Char*)m_start, len);
}

Void TokenUtil::seek(const Token* marker) {    
    if (m_start <= marker->m_token && m_end >= marker->m_token) {
        m_start = marker->m_token;
    }
}

Void TokenUtil::seekAfter(const Token* marker) {
    const Char* psz = marker->m_token + marker->m_size;
    
    if (m_start <= psz && m_end >= psz) {
        m_start = psz;
    }
}

Void TokenUtil::trancate(const Token* marker) {    
    if (m_start <= marker->m_token && m_end >= marker->m_token) {
        m_end = marker->m_token;
    }
}

Void TokenUtil::trancateAfter(const Token* marker) {
    const Char* psz = marker->m_token + marker->m_size;
    
    if (m_start <= psz && m_end >= psz) {
        m_end = psz;
    }
}

Void TokenUtil::addHead(Int32 n) {
    m_start += n;
    
    if (m_start > m_end) {
        m_start = m_end;
    }
}

Void TokenUtil::subTail(Int32 n) {
    m_end -= n;

    if (m_end < m_start) {
        m_end = m_start;
    }
}

Bool TokenUtil::nextToken(const Char* sep, Token* next) {
    Bool bOk = FALSE;
    Token t = DEF_EMPTY_TOKEN;

    skipHead(sep);
    if (!isEol()) {
        bOk = findFirstOf(sep, &t);
        if (bOk) {
            cutBefore(&t, next);

            seekAfter(&t);
        } else {
            copy(next);
            setEol();
        }

        return TRUE;
    } else {        
        setToken(next, NULL, 0);
        return FALSE;
    }
}

Bool TokenUtil::nextToken(Char sep, Token* next) {
    Int32 len = 0;
    const Char* psz = NULL;

    while (!isEol() && sep == *m_start) {
        ++m_start;
    }
    
    if (!isEol()) {
        psz = m_start;

        /* skip not sep */
        ++m_start;

        while (!isEol() && sep != *m_start) {
            ++m_start;
        }

        len = (Int32)(m_start - psz);
        setToken(next, (Char*)psz, len);

        if (!isEol()) {
            /* skip the sep */
            ++m_start;
        }

        return TRUE;
    } else {        
        setToken(next, NULL, 0);
        return FALSE;
    }
}

Void TokenUtil::skipHead(const Char* sep) {
    Bool bOk = FALSE;
    Token t = DEF_EMPTY_TOKEN;
    
    bOk = findFirstNotOf(sep, &t);
    if (bOk) {
        seek(&t);
    } else {
        setEol();
    }
}

Void TokenUtil::skipTail(const Char* sep) {
    Bool bOk = FALSE;
    Token t = DEF_EMPTY_TOKEN;
    
    bOk = findLastNotOf(sep, &t);
    if (bOk) {
        trancateAfter(&t);
    } else {
        setEol();
    }
}

Void TokenUtil::skipBlank() {
    skipHead(DEF_BLANK_STR);
}

Bool TokenUtil::nextWord(Token* next) {
    Bool bOk = FALSE;

    bOk = nextToken(DEF_BLANK_STR, next);
    return bOk;
}

Int32 TokenUtil::cmpToken(const Token* token1, const Token* token2) {
    Int32 ret = 0;
    Int32 len = 0;

    if (0 < token1->m_size && 0 < token2->m_size) {
        len = token1->m_size > token2->m_size ? 
            token1->m_size : token2->m_size; 
        
        ret = strncmp(token1->m_token, token2->m_token, len);
    } else {
        /* at least one is null */
        ret = token1->m_size - token2->m_size;
    }
    
    return ret;
}

Int32 TokenUtil::caseCmpToken(const Token* token1, const Token* token2) {
    Int32 ret = 0;
    Int32 len = 0;

    if (0 < token1->m_size && 0 < token2->m_size) {
        len = token1->m_size > token2->m_size ? 
            token1->m_size : token2->m_size; 
        
        ret = strncasecmp(token1->m_token, token2->m_token, len);
    } else {
        /* at least one is null */
        ret = token1->m_size - token2->m_size;
    }
    
    return ret;
}

Int32 TokenUtil::cmpStr(const Token* token, const Char* needle) {
    Int32 ret = 0;
    Int32 len = strlen(needle);

    if (0 < len && 0 < token->m_size) {
        if (len < token->m_size) {
            len = token->m_size;
        }
        
        ret = strncmp(token->m_token, needle, len);
    } else {
        ret = token->m_size - len;
    }

    return ret;
}

Int32 TokenUtil::caseCmpStr(const Token* token, const Char* needle) {
    Int32 ret = 0;
    Int32 len = strlen(needle);

    if (0 < len && 0 < token->m_size) {
        if (len < token->m_size) {
            len = token->m_size;
        }
        
        ret = strncasecmp(token->m_token, needle, len);
    } else {
        ret = token->m_size - len;
    }

    return ret;
}

Bool TokenUtil::isFall(const Char* delim, Int32 delimLen, Char ch) {
    for (int i=0; i<delimLen; ++i) {
        if (delim[i] == ch) {
            return TRUE;
        }
    }

    return FALSE;
}

Bool TokenUtil::isBlank(Char ch) {
    Bool bOk = FALSE;

    bOk = isFall(DEF_BLANK_STR, DEF_BLANK_STR_SIZE, ch); 
    return bOk;
}

Bool TokenUtil::isSpace(Char ch) {
    Bool bOk = FALSE;

    bOk = isFall(DEF_LWS_STR, DEF_LWS_STR_SIZE, ch); 
    return bOk;
}

Void TokenUtil::stripToken(Token* token) {
    Int32 len = 0;
    Char* start = NULL;
    Char* end = NULL;

    if (0 < token->m_size) {
        start = token->m_token;
        end = start + token->m_size;

        while (isSpace(*start) && start < end) {
            ++start;
        }

        if (start < end) {
            /* locate to the last char */
            --end; 
            while (isSpace(*end) && start <= end) {
                --end;
            }

            /* skip one */
            ++end;
            
            len = (Int32)(end - start);
            setToken(token, start, len); 
        } else {
            setToken(token, NULL, 0);
        } 
    }
}

Int32 TokenUtil::toNum(const Token* token) {
    Int32 n = 0;
    Char buf[COMM_BUFFER_SIZE] = {0};

    getToken(token, buf, COMM_BUFFER_SIZE);
    
    n = atoi(buf);
    return n;
}

