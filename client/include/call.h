#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#ifndef CHAT_CALL_H
#define CHAT_CALL_H

//spellchecker: ignore stringize

#define STRINGIZE2(x) #x
#define STRINGIZE(x) STRINGIZE2(x)

#define TRACE __FILE__ ":" STRINGIZE(__LINE__)
#define CALL(cond, err)                   \
    {                                     \
        if (cond)                         \
        {                                 \
            perror(#cond " , at " TRACE); \
            err;                          \
        }                                 \
    }

#define P_CALL(func, err)                 \
    {                                     \
        int __res = func;                 \
        if (__res != 0)                   \
        {                                 \
            errno = __res;                \
            perror(#func " , at " TRACE); \
            err;                          \
        }                                 \
    }

void *mallocLog(size_t size);
void freeLog(void *ptr);

#endif