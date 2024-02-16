#include "Poller.h"
#include "Channel.h"

Poller::Poller(EventLoop *loop):m_ownerLoop(loop)
{
}

Poller::~Poller()
{
}

bool Poller::hsaChannel(Channel *channel) const
{
    auto it = m_channels.find(channel->fd());
    return it != m_channels.end() && it->second == channel;
}

