#pragma once

#include "noncopyable.h"

#include <functional>
#include <string>
#include <vector>
#include <memory>


class EventLoop;
class EventLoopThread;


class EventLoopThreadPool : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    EventLoopThreadPool(EventLoop *baseLoop,const std::string &name);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads) { m_numThread = numThreads; }

    void start(const ThreadInitCallback &cb = ThreadInitCallback());

    // 如果工作在多线程中，baseloop默认轮询的方式分配channel给子loop
    EventLoop *getNextLoop();

    std::vector<EventLoop *> getAllLoops();

    bool started() const { return m_started; }

    const std::string &name() const { return m_name; }
private:
    EventLoop *m_baseLoop;   // EventLoop   最基础的一个loop，假如没有设置线程数量则默认单线程
    std::string m_name;
    bool m_started;
    int m_numThread;
    int m_next;
    std::vector<std::unique_ptr<EventLoopThread>> m_threads;
    std::vector<EventLoop *>m_loops;
};

