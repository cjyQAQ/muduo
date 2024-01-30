#pragma once 

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"
#include <functional>

class EventLoop;

class Acceptor : noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int sockfd,const InetAddress&)>;    

    Acceptor(EventLoop *loop,const InetAddress &listenAddr,bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback &cb) { m_newConnectionCallback = std::move(cb); }

    bool listenning() const { return m_listenning;}
    void listen();
private:
    void handleRead();

    EventLoop *m_loop;    //acceptor 用的就是用户定义的baseloop也就是mainloop
    Socket m_acceptSocket;
    Channel m_acceptChannel;
    NewConnectionCallback m_newConnectionCallback;
    bool m_listenning;

};
 
