#include "Logger.h"
#include "Timestamp.h"

#include <iostream>

Logger &Logger::GetInstance()
{
    static Logger log;
    return log;
}

void Logger::setLogLevel(int level)
{
    m_loglevel = level;
}

// 格式 [级别信息] time:msg
void Logger::log(std::string msg)
{
    switch (m_loglevel)
    {
    case INFO:
        std::cout<<"[INFO] ";
        break;
    case ERROR:
        std::cout<<"[ERROR] ";
        break;
    case FATAL:
        std::cout<<"[FATAL] ";
        break;
    case DEBUG:
        std::cout<<"[DEBUG] ";
        break;
    default:
        break;
    }
    std::cout<<Timestamp::now().toString() << " : " <<
     msg << std::endl;
}
