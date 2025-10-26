#ifndef TEXTERR_H
#define TEXTERR_H

#include <stdint.h>

typedef enum {

    ERR_EOF = -3,
    ERR_NULL = -2,
    ERR_MEM = -1,
    ERR_NONE = 0,

} textErr;

static inline const char* textErr_tostr(textErr err) {
    switch (err) {
        case ERR_EOF:  return "End of file";
        case ERR_NULL: return "Null pointer";
        case ERR_MEM:  return "Memory allocation error";
        case ERR_NONE: return "No error";
        default:       return "Unknown error";
    }
}

#endif
