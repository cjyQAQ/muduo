#include "Poller.h"
#include <stdlib.h>
#include "EpollPoller.h"

Poller *Poller::newDefaultPoller(EventLoop *loop)
{
    if(::getenv("MUDUO_USE_POLL"))
    {
        return nullptr;
    }
    else
    {
        // TODO
        return new EpollPoller(loop);
    }
}
