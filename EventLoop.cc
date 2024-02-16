#include "EventLoop.h"
#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include "Logger.h"
#include <errno.h>
#include "Poller.h"
#include "Channel.h"

//防止一个线程创建多个eventloop
__thread EventLoop *t_loopInThisThread = nullptr;

// 定义默认的POller IO 复用接口的超时时间
const int kPollTimeMs = 10000;


int createEventfd()
{
    int evtfd = ::eventfd(0,EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd<0)
    {

        LOG_FATAL("eventfd error:%d\n",errno);
    }
    return evtfd;
}
EventLoop::EventLoop()
            :m_looping(false),
            m_quit(false),
            m_callingPendingFunctors(false),
            m_threadId(CurrentThread::tid()),
            m_poller(Poller::newDefaultPoller(this)),
            m_wakeupfd(createEventfd()),
            m_wakeupChannel(new Channel(this,m_wakeupfd))
{
    LOG_INFO("EventLoop create %p in thread %d\n",this,m_threadId);
    if(t_loopInThisThread)
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d\n",t_loopInThisThread,m_threadId);
    }
    else
    {
        t_loopInThisThread = this;
    }

    // 设置wakeupfd的事件类型和发生事件后的回调操作
    m_wakeupChannel->setReadCallback(std::bind(&EventLoop::handleRead,this));
    // 每个eventloop都将监听wakeupchannel的EPOLLIN事件
    m_wakeupChannel->enableReading();
}

EventLoop::~EventLoop()
{
    m_wakeupChannel->disableAll();
    m_wakeupChannel->remove();
    ::close(m_wakeupfd);
    t_loopInThisThread = nullptr;
}

void EventLoop::loop()
{
    m_looping = true;
    m_quit = false;
    LOG_INFO("EventLoop %p start looping\n",this);

    while(!m_quit)
    {
        m_activeChannels.clear();
        // 监听两类fd，一种是clientfd，一种是wakeupfd
        m_pollerReturnTime = m_poller->poll(kPollTimeMs,&m_activeChannels);
        for( Channel *channel : m_activeChannels)
        {
            // Poller监听那些channel发生事件了，然后上报给eventloop，通知channel处理相应的事件
            channel->handleEvent(m_pollerReturnTime);
        }
        // 执行当前EventLoop事件循环需要处理的回调操作
        /*
            IO 线程mainLoop主要接收新用户的连接，返回一个fd，使用一个channel来打包。 已经连接的channel就分发给subloop
            main 事先注册一个回调cb(需要subloop来执行)   wakeup subloop 后执行下面的方法，执行之前mainloop注册的cb操作
        */
        doPendingFunctors();
    }
    LOG_INFO("EventLoop %p stop looping...\n",this);
    m_looping = false;
}

// 1.loop在自己的线程中调用loop
// 2.在其他线程中调用了quit，比如在subloop（woker）中调用了mainloop的quit
void EventLoop::quit()
{
    m_quit = true;
    if(!isInLoopThread())
    {
        wakeup();
    }
}

void EventLoop::runInLoop(Functor cb)
{
    if(isInLoopThread())  // 当前的loop线程中，执行cb
    {
        cb();
    }
    else  // 在非当前loop线程中执行cb，需要唤醒loop所在的线程，执行cb
    {
        queueInLoop(std::move(cb));  
    }
}

void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_pendingFunctors.emplace_back(cb);
    }

    // 唤醒相应的需要执行上面回调操作的loop线程
    // || m_callingPendingFunctors 的作用是：当前的loop正在执行回调，但是loop中又来了新的回调，那么为了避免当前loop执行完后阻塞，使用wakeup将其唤醒来执行该回调
    if(!isInLoopThread() || m_callingPendingFunctors)
    {
        wakeup();  // 唤醒loop所在线程
    }
}

// 向wakeupfd写一个数据,wakeupchannel就会发生读事件，当前loop线程就会被唤醒
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = ::write(m_wakeupfd,&one,sizeof(one));
    if(n != sizeof(one))
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8\n",one);
    }
}

void EventLoop::updateChannel(Channel *channel)
{
    m_poller->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    m_poller->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel)
{
    return m_poller->hsaChannel(channel);;
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(m_wakeupfd,&one,sizeof(one));
    if( n != sizeof(one))
    {
        LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8\n",one);
    }
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    m_callingPendingFunctors = true;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        functors.swap(m_pendingFunctors);
    }

    for(const Functor &functor : functors)
    {
        functor();    // 执行当前loop需要回调的操作
    }
    m_callingPendingFunctors = false;
}
