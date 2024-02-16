#include "Channel.h"
#include "EventLoop.h"
#include <sys/epoll.h>
#include "Logger.h"

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;



Channel::Channel(EventLoop *loop, int fd):m_loop(loop),m_fd(fd),m_event(0),m_revent(0),m_index(-1),m_tied(false)
{
    
}

Channel::~Channel()
{
}

void Channel::handleEvent(Timestamp recvtime)
{
    std::shared_ptr<void> guard;
    if(m_tied)
    {
        guard = m_tie.lock();
        if(guard)
        {
            handleEventWithGuard(recvtime);
        }
    }
    else
    {
        handleEventWithGuard(recvtime);
    }
}

void Channel::tie(const std::shared_ptr<void> &obj)
{
    m_tie = obj;
    m_tied = true;
}

// 在channel所属的eventloop中将当前的channel删除掉
void Channel::remove()
{
    // TODO
    m_loop->removeChannel(this);
}

// 当改变channel所表示fd的事件后，update负责在poller里面更改fd相应的事件epoll_ctl
void Channel::update()
{
    // 通过channel所属的eventloop，调用poller的相应的方法，注册fd的event事件
    // TODO
    m_loop->updateChannel(this);
}

// 根据poller通知的channel发生的具体事件，由channel负责调用具体的回调操作
void Channel::handleEventWithGuard(Timestamp recvtime)
{
    LOG_INFO("channel handleevent revent:%d\n",m_revent);
    if((m_revent & EPOLLHUP) && !(m_revent &EPOLLIN))
    {
        if(m_closeCallback)
        {
            m_closeCallback();
        }
    }

    if(m_revent & EPOLLERR)
    {
        if(m_errorCallback)
        {
            m_errorCallback();
        }
    }

    if(m_revent & EPOLLIN)
    {
        if(m_readCallback)
        {
            m_readCallback(recvtime);
        }
    }
    if(m_revent & EPOLLOUT)
    {
        if(m_writeCallback)
        {
            m_writeCallback();
        }
    }
}
