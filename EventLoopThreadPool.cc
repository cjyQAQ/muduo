#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"
#include <memory>

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, const std::string &name)
                    :m_baseLoop(baseLoop),
                     m_name(name),
                     m_started(false),
                     m_numThread(0),
                     m_next(0)
{

}

EventLoopThreadPool::~EventLoopThreadPool()
{

}

void EventLoopThreadPool::start(const ThreadInitCallback &cb)
{
    m_started = true;

    for(int i = 0 ;i<m_numThread;i++)
    {
        char buf[m_name.size() + 32];
        snprintf(buf,sizeof(buf),"%s%d",m_name.c_str(),i);
        EventLoopThread *t = new EventLoopThread(cb,buf);
        m_threads.emplace_back(std::unique_ptr<EventLoopThread>(t));
        m_loops.emplace_back(t->startLoop());   // 创建线程，绑定一个新的EventLoop，并返回该loop的地址
    }
    // 说明只有一个线程运行着baseloop
    if(m_numThread == 0 && cb)
    {   
        cb(m_baseLoop);
    }  
}

EventLoop *EventLoopThreadPool::getNextLoop()
{
    EventLoop *loop = m_baseLoop;
    if(!m_loops.empty())    // 通过轮询获取下一个处理事件的loop
    {
        loop = m_loops[m_next];
        ++m_next;
        if(m_next >= m_loops.size())
        {
            m_next = 0;
        }
    }
    return loop;
}

std::vector<EventLoop *> EventLoopThreadPool::getAllLoops()
{
    if(m_loops.empty())
    {
        return std::vector<EventLoop *>(1,m_baseLoop);
    }
    else
    {
        return m_loops;
    }
}
