/*
 * File:   messages.h
 * Author: yuri
 *
 * Created on March 18, 2011, 11:01 AM
 */

#ifndef MESSAGES_H
#define MESSAGES_H
#define MSG_NEVER    0
#define MSG_ERROR    1
#define MSG_NORMAL   2
#define MSG_VERBOSE  3

extern int MSG_LEVEL;
static const char *str_MSG_LEVEL[4] = {"MSG_NEVER", "MSG_ERROR", "MSG_NORMAL", "MSG_VERBOSE" };
//char **str_MSG_LEVEL;
#define PRINT(LEVEL, ...) \
    if(LEVEL <= MSG_LEVEL){ \
        if(MSG_LEVEL == MSG_VERBOSE){ \
            printf("(%s)", str_MSG_LEVEL[LEVEL]);  \
            printf("[%s:%04d] ", __FILE__, __LINE__);  \
        } \
        printf(__VA_ARGS__); \
        fflush(stdout);}

#else

#endif  /* MESSAGES_H */

