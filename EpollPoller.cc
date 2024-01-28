#include "EpollPoller.h"
#include "Logger.h"
#include "Channel.h"
#include <errno.h>
#include <unistd.h>
#include <string.h>

// channel未添加到poller中
const int kNew = -1;     // channel的成员m_index初始化为-1
// channel已经添加到poller中
const int kAdded = 1;
// channel从poller中删除
const int kDeleted = 2;

EpollPoller::EpollPoller(EventLoop *loop):Poller(loop),m_epollfd(::epoll_create1(EPOLL_CLOEXEC)),m_events(kInitEventListSize)
{
    if(m_epollfd < 0)
    {
        LOG_FATAL("epoll_create error %d\n",errno);
    }
}

EpollPoller::~EpollPoller()
{
    ::close(m_epollfd);
}

Timestamp EpollPoller::poll(int timeout, ChannelList *activeChannels)
{
    LOG_INFO("func = %s => fd total count: %lu\n",__FUNCTION__,m_channels.size());

    int numEvents = ::epoll_wait(m_epollfd,&*m_events.begin(),static_cast<int>(m_events.size()),timeout);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());

    if(numEvents > 0)
    {
        LOG_INFO("%d events happend\n",numEvents);
        fillActivateChannels(numEvents,activeChannels);
        if(numEvents == m_events.size())
        {
            m_events.resize(m_events.size() * 2);
        }
    }
    else if(numEvents == 0)
    {
        LOG_DEBUG("%s timeout!\n",__FUNCTION__);
    }
    else
    {
        if(saveErrno != EINTR)
        {
            errno = saveErrno;
            LOG_ERROR("EpollPoller::poll() err!");
        }
    }
    return now;
}

// channel uodate remove 会最终是通过eventloop作为中间人调用poller中的updatechannel和removechannel

/*
            EventLoop
    ChannelList      Poller
                    ChannelMap <fd,channel *>

*/
void EpollPoller::updateChannel(Channel *channel)
{
    const int index = channel->index();
    LOG_INFO("func = %s => fd = %d events = %d index = %d\n",__FUNCTION__,channel->fd(),channel->event(),channel->index());
    if(index == kNew || index == kDeleted)
    {
        if(index == kNew)
        {
            int fd = channel->fd();
            m_channels[fd] = channel;
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD,channel);
    }
    else  // 已经注册过
    {
        int fd = channel->fd();
        if(channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL,channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD,channel);
        }
    }

}

void EpollPoller::removeChannel(Channel *channel)
{
    int fd = channel->fd();
    LOG_INFO("func = %s => fd = %d\n",__FUNCTION__,fd);
    m_channels.erase(fd);
    int index = channel->index();
    if(index == kAdded)
    {
        update(EPOLL_CTL_DEL,channel);
    }
    channel->set_index(kNew);
}

void EpollPoller::fillActivateChannels(int numEvents, ChannelList *activeChannels) const
{
    for(int i = 0;i<numEvents;i++)
    {
        Channel *channel = static_cast<Channel *>(m_events[i].data.ptr);
        channel->set_revent(m_events[i].events);
        activeChannels->push_back(channel);    // eventloop 就拿到了它的poller给它返回的所有发生事件的channel列表
    }
}

// epoll_ctl add/remove/modify
void EpollPoller::update(int operation , Channel *channel)
{
    struct epoll_event event;
    memset(&event,0,sizeof(event));
    event.data.ptr = channel;
    event.events = channel->event();
    int fd = channel->fd();
    if(::epoll_ctl(m_epollfd,operation,fd,&event) < 0)
    {
        if(operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error:%d\n",errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl add/mod error:%d\n",errno);
        }
    }
}
