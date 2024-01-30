# pragma once

#include "noncopyable.h"
#include <functional>
#include <vector>
#include <atomic>
#include "Timestamp.h"
#include "CurrentThread.h"
#include <memory>
#include <mutex>


class Channel;
class Poller;

// 事件循环类 主要包含了两个大模块 Channel Poller(epoll的抽象)
class EventLoop
{
public:
    using Functor = std::function<void()>;
    EventLoop();
    ~EventLoop();
    
    // 开启事件循环
    void loop();
    // 退出事件循环
    void quit();

    Timestamp pollReturnTimr() const { return m_pollerReturnTime; }

    // 在当前loop中执行
    void runInLoop(Functor cb);
    // 把cb放在队列中，唤醒loop所在的线程，执行cb
    void queueInLoop(Functor cb);

    // 用来唤醒loop所在的线程
    void wakeup();

    // eventloop 调用 poller方法
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    // 判断eventloop对象是否在自己的线程里
    bool isInLoopThread() const { return m_threadId == CurrentThread::tid(); }
private:
    // wakeup
    void handleRead(); 

    //执行回调
    void doPendingFunctors();

    using ChannelList = std::vector<Channel *>;

    std::atomic_bool m_looping;    // 正在循环
    std::atomic_bool m_quit;     //标识退出loop循环

    const pid_t m_threadId;      // 记录当前loop所在的线程

    Timestamp m_pollerReturnTime; // poller返回发生事件的channel的时间点
    std::unique_ptr<Poller> m_poller;

    int m_wakeupfd;   // 主要作用：当mainLoop获取一个新用户的channel，通过轮询算法选择一个subloop，通过该成员唤醒subloop
    std::unique_ptr<Channel> m_wakeupChannel;

    ChannelList m_activeChannels;
    // Channel *m_currentActiveChannel;


    std::atomic_bool m_callingPendingFunctors;  // 标识当前loop是否有需要执行的回调操作
    std::vector<Functor>  m_pendingFunctors;   //存储loop需要执行的所有回调操作
    std::mutex m_mutex;     //用来保护上面的vector
};

