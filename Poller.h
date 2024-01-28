#pragma once

#include "noncopyable.h"
#include "Timestamp.h"


#include <iostream>
#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;
// muduo 中多路事件分发器的核心IO复用模块
class Poller : noncopyable
{
public:
    Poller(EventLoop *loop);
    virtual ~Poller();
    using ChannelList = std::vector<Channel *>;

    // 保留统一的接口
    virtual Timestamp poll(int timeout,ChannelList *activeChannels) = 0;
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;

    // 判断参数channel是否在当前Poller中
    bool hsaChannel(Channel *channel) const;
    // 可以通过该接口获取默认的IO复用的具体实现
    static Poller *newDefaultPoller(EventLoop *loop);  // 为什么不写在poller.cc中 poller是基类，Epoll和Poll是派生类，基类包含派生类的头文件等设计不好
protected:
    using ChannelMap = std::unordered_map<int,Channel *>;
    ChannelMap m_channels;

private:
    EventLoop *m_ownerLoop; // 定义poller所属与的事件循环eventloop
};


