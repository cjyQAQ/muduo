# pragma once
#include "noncopyable.h"
#include "Timestamp.h"
#include <functional>
#include <memory>
class EventLoop;


/*
    channel 是通道，封装了sockfd和其感兴趣的event，如epollin，epollout等事件
    还绑定了poller返回的具体事件

*/

class Channel : noncopyable
{
public:
    Channel(EventLoop *loop,int fd);
    ~Channel();
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;
    // 得到；poller通知以后，处理事件的回调
    void handleEvent(Timestamp recvtime);
    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb) { m_readCallback = std::move(cb); }
    void setWriteCallback(EventCallback cb) { m_writeCallback = std::move(cb); }
    void setCloseCallback(EventCallback cb) { m_closeCallback = std::move(cb); }
    void setErrorCallback(EventCallback cb) { m_errorCallback = std::move(cb); }

    // 防止当channel被手动move掉，channel还在执行回调操作
    void tie(const std::shared_ptr<void> &);

    int fd() const { return m_fd; }
    int event() const { return m_event; }
    void  set_revent(int revt) { m_revent = revt; }
    
    //设置fd相应的事件状态
    void enableReading() { m_event |= kReadEvent; update(); }
    void disableReading() { m_event &= ~kReadEvent; update(); }
    void enableWriting() { m_event |= kWriteEvent; update(); }
    void disableWriting() { m_event &= ~kWriteEvent; update(); }
    void disableAll() { m_event = kNoneEvent; update(); }

    //返回fd当前的事件状态
    bool isNoneEvent() const { return m_event == kNoneEvent; }
    bool isWriting() const { return m_event & kWriteEvent; }
    bool isReading() const { return m_event & kReadEvent; }

    int index() { return m_index; }
    void set_index(int idx) { m_index = idx; }

    EventLoop *ownerLoop() { return m_loop; }
    void remove();

private:
    void update();
    void handleEventWithGuard(Timestamp recvtime);

private:
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;
    EventLoop *m_loop;       // 事件循环
    const int m_fd;         //fd，poller监听的对象
    int m_event;           //注册fd感兴趣的事件
    int m_revent;         //poller返回的具体发生的事件
    int m_index;

    std::weak_ptr<void> m_tie;
    bool m_tied;

    // 因为channel通道里面可以获得fd最终发生的具体revent，所以负责调用具体事件的回调操作
    ReadEventCallback m_readCallback;
    EventCallback m_writeCallback;
    EventCallback m_closeCallback;
    EventCallback m_errorCallback;
};

