#ifndef __ENG_LOG_H__
#define __ENG_LOG_H__

#include <stdio.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG     "WLNPI"
//#define ENG_LOG(...) do{} while(0)
#define ENG_LOG(format, ...) printf("%s "format, LOG_TAG, ##__VA_ARGS__)

#endif
