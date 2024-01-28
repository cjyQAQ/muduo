# pragma once
#include <string>

#include "noncopyable.h"

// LOG_INFO("%s %d", arg1,arg2 )
#define LOG_INFO(logmsgFormat, ...)\
    do\
    {                                                 \
        Logger &logger = Logger::GetInstance();       \
        logger.setLogLevel(INFO);                     \
        char buf[1024] = {0};                         \
        snprintf(buf,1024,logmsgFormat,##__VA_ARGS__);\
        logger.log(buf);                              \
    } while (0);

#define LOG_ERROR(logmsgFormat, ...)\
    do\
    {                                                 \
        Logger &logger = Logger::GetInstance();       \
        logger.setLogLevel(ERROR);                     \
        char buf[1024] = {0};                         \
        snprintf(buf,1024,logmsgFormat,##__VA_ARGS__);\
        logger.log(buf);                              \
    } while (0);

#define LOG_FATAL(logmsgFormat, ...)\
    do\
    {                                                 \
        Logger &logger = Logger::GetInstance();       \
        logger.setLogLevel(FATAL);                     \
        char buf[1024] = {0};                         \
        snprintf(buf,1024,logmsgFormat,##__VA_ARGS__);\
        logger.log(buf);                              \
    } while (0);

#ifdef MUDEBUG
#define LOG_DEBUG(logmsgFormat, ...)\
    do\
    {                                                 \
        Logger &logger = Logger::GetInstance();       \
        logger.setLogLevel(DEBUG);                     \
        char buf[1024] = {0};                         \
        snprintf(buf,1024,logmsgFormat,##__VA_ARGS__);\
        logger.log(buf);                              \
    } while (0);
#else
    #define LOG_DEBUG(logmsgFormat, ...)
#endif


// 定义日志级别 INFO ERROR FATAL DEBUG
enum LogLevel
{
    INFO,
    ERROR,
    FATAL,
    DEBUG,
};


// 日志类
class Logger : noncopyable
{
private:
    int m_loglevel;
    Logger(){}
    ~Logger(){}
public:
    static Logger &GetInstance();
    // 设置日志级别
    void setLogLevel(int level);
    // 写日志
    void log(std::string msg);
};


