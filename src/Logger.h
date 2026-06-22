// FILE: Logger.h
#pragma once

#include <stdio.h>
#include <time.h>
#include <sys/syscall.h>
#include <unistd.h>

#define ENABLE_DEBUG_LOG 0   //设置日志的开关

// A simple logging macro that includes a timestamp and thread ID.
#if ENABLE_DEBUG_LOG
    #define LOG(...) do { \
        char time_buf[64]; \
        time_t now = time(0); \
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", localtime(&now)); \
        printf("[%s] [tid:%ld] ", time_buf, syscall(SYS_gettid)); \
        printf(__VA_ARGS__); \
        printf("\n"); \
        fflush(stdout); \
    } while (0)
#else
    #define LOG(...) do {} while(0)

#endif