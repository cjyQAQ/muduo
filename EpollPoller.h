#include "Poller.h"
#include <vector>
#include <sys/epoll.h>


/*
    epoll
    epoll_create
    epoll_ctl
    epoll_wait
*/

class EpollPoller : public Poller
{
public:
    EpollPoller(EventLoop *loop);
    ~EpollPoller() override;
    // 重写基类的抽象方法
    Timestamp poll(int timeout,ChannelList *activeChannels) override;
    void updateChannel(Channel *channel) override;
    void removeChannel(Channel *channel) override;

private:
    static const int kInitEventListSize = 16;
    // 填写活跃的链接
    void fillActivateChannels(int numEvents,ChannelList *activeChannels) const;
    // 更新channel通道
    void update(int operation , Channel *channel);

    using EventList = std::vector<epoll_event>;
    int m_epollfd;
    EventList m_events;
};

