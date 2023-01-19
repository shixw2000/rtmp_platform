#include<regex.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"config.h"
#include"globaltype.h"


Void ConfParser::stripComment(Char* text) {
    Char* pch = NULL;

    pch = strchr(text, '#');
    if (NULL != pch) {
        /* at position of 0 or follow a space */
        if (pch == text || isspace(*(pch-1))) {
            *pch = '\0';
        }
    }
}

Void ConfParser::strip(Char* text) {
    Int32 len = 0;
    Char* beg = NULL;
    Char* end = NULL;

    len = strnlen(text, MAX_LINE_LEN-1);
    beg = text;
    end = text + len;

    while (isspace(*beg) && beg < end) {
        ++beg;
    }

    while (beg < end && isspace(*(end-1))) {
        --end;
    }

    if (beg < end) {
        len = end - beg;
        memmove(text, beg, len);
        text[len] = '\0';
    } else {
        text[0] = '\0';
    }
}

int ConfParser::getKeyVal(const Char* line, Char* key, Int32 key_len,
    Char* val, Int32 val_len) {
    static const Char DEF_STR_PATTERN[] = 
        "^[[:space:]]*([_[:alnum:]]*)[[:space:]]*=[[:space:]]*\"(.*)\"[[:space:]]*$"; 
    int ret = 0;
    int len = 0;
    regex_t reg;
    regmatch_t matchs[3];  
    
    ret = regcomp(&reg, DEF_STR_PATTERN, REG_EXTENDED | REG_NEWLINE);
    if (0 != ret) {
        return -1;
    }

    do {
        ret = regexec(&reg, line, 3, matchs, 0);
        if (0 != ret) { 
            break;
        }
        
        len = (int)(matchs[1].rm_eo-matchs[1].rm_so);
        if (len < key_len) {
            
            memcpy(key, &line[ matchs[1].rm_so ], len);
            key[len] = '\0';
        } else {
            ret = -1;
            break;
        }

        len = (int)(matchs[2].rm_eo-matchs[2].rm_so);
        if (len < val_len) {            
            memcpy(val, &line[ matchs[2].rm_so ], len);
            val[len] = '\0';
        } else {
            ret = -1;
            break;
        }

        ret = 0;
    } while (0);

    regfree(&reg);
    return ret;
}

Bool ConfParser::getSection(Char line[]) {
    Int32 len = 0;

    len = strnlen(line, MAX_LINE_LEN);
    if (2 < len) {
        if ('[' == line[0] && ']' == line[len-1]) {
            len -= 2;
            memmove(line, &line[1], len);
            line[len] = '\0';
            
            return TRUE;
        } else {
            return FALSE;
        }
    } else {
        return FALSE;
    }
}

Int32 ConfParser::parse(const Char* path) {
    Int32 ret = 0;
    Int32 lineno = 0;
    Bool bSec = TRUE;
    FILE* hd = NULL;
    Char line[MAX_LINE_LEN] = {0};
    Char key[MAX_LINE_LEN] = {0};
    Char val[MAX_LINE_LEN] = {0};
    Char* psz = NULL;
    typeConfItr itrConf;

    m_conf.clear();

    hd = fopen(path, "rb");
    if (NULL != hd) { 
        itrConf = m_conf.end();
        
        psz = fgets(line, MAX_LINE_LEN, hd);
        while (NULL != psz) {
            ++lineno;
            
            stripComment(line);
            strip(line);

            /* ignore empty or comment line */
            if ('#' != line[0] && '\0' != line[0]) {
                bSec = getSection(line);
                if (!bSec) {
                    if (itrConf != m_conf.end()) {
                        ret = getKeyVal(line, key, MAX_LINE_LEN, val, MAX_LINE_LEN);
                        if (0 == ret) {
                            typeMap& imap = itrConf->second;

                            imap[key] = val;
                        } else {
                            /* invalid item */
                            ret = -2;
                            break;
                        }
                    } else {
                        /* outside of a section */
                        ret = -3;
                        break;
                    }
                } else {
                    itrConf = m_conf.find(line);
                    if (itrConf == m_conf.end()) {
                        m_conf[line] = typeMap();
                        itrConf = m_conf.find(line);
                    } else {
                        /* duplicated section */
                        ret = -4;
                        break;
                    }
                }
            }
            
            psz = fgets(line, MAX_LINE_LEN, hd);
        }

        fclose(hd);
    } else {
        ret = -1;
    }

    if (0 == ret) {
        LOG_INFO("parse_conf| path=%s| lineno=%d| msg=ok|",
            path, lineno);
        
        return 0;
    } else {
        LOG_ERROR("parse_conf| ret=%d| path=%s| lineno=%d| msg=parse error|",
            ret, path, lineno);
        return ret;
    }
}

Int32 ConfParser::getKeyStr(typeMap& imap, const Char key[],
    Char val[], Int32 maxlen) {
    Int32 size = 0;
    typeMapItr itrKey;

    itrKey = imap.find(key);
    if (imap.end() != itrKey) {
        const typeStr& str = itrKey->second;

        size = (Int32)str.size();
        if (size < maxlen) {
            str.copy(val, size);
            val[size] = '\0';

            return 0;
        } else {
            return -1;
        }
    } else {
        return -1;
    } 
}

Int32 ConfParser::getKeyInt(typeMap& imap, const Char key[], Int32* val) {
    Int32 ret = 0;
    Char buf[MAX_LINE_LEN] = {0};

    ret = getKeyStr(imap, key, buf, MAX_LINE_LEN);
    if (0 == ret) {
        if (isdigit(buf[0])) {
            *val = atoi(buf);
            return 0;
        } else {
            return -1;
        }
    } else {
        return -1;
    } 
}

Int32 ConfParser::getParamInt(const Char secName[], 
    const Char key[], Int32* val) {
    Int32 ret = 0;
    typeConfItr itrConf;

    itrConf = m_conf.find(secName);
    if (m_conf.end() != itrConf) {
        typeMap& imap = itrConf->second;
        
        ret = getKeyInt(imap, key, val);
        if (0 == ret) {
            LOG_INFO("get_param_int| sec=%s| key=%s| val=%d|",
                secName, key, *val);

            return 0;
        } else {
            LOG_ERROR("get_param_int| sec=%s| key=%s| msg=invalid|",
                secName, key);
            return -1;
        }      
    } else {
        return -1;
    }
}

Int32 ConfParser::getParamStr(const Char secName[],
    const Char key[], Char val[], Int32 maxlen) {
    Int32 ret = 0;
    typeConfItr itrConf;

    itrConf = m_conf.find(secName);
    if (m_conf.end() != itrConf) {
        typeMap& imap = itrConf->second;
        
        ret = getKeyStr(imap, key, val, maxlen);
        if (0 == ret) {
            LOG_INFO("get_param_str| sec=%s| key=%s| val=%s|",
                secName, key, val);

            return 0;
        } else {
            LOG_ERROR("get_param_str| sec=%s| key=%s| msg=invalid|",
                secName, key);
            return -1;
        }     
    } else {
        return -1;
    }
}

