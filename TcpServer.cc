#include "TcpServer.h"
#include "Logger.h"
#include "TcpConnection.h"

#include <strings.h>

static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    if(loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainLoop is null\n",__FILE__,__FUNCTION__,__LINE__);
    }
    return loop;
}
    

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &name, Option option)
            :m_loop(CheckLoopNotNull(loop)),
             m_ipPort(listenAddr.toIpPort()),
             m_name(name),
             m_acceptor(new Acceptor(loop,listenAddr,option == kReusePort)),
             m_threadPool(new EventLoopThreadPool(loop,m_name)),
             m_connectionCallback(),
             m_messageCallback(),
             m_nextConnId(1),
             m_started(0)
              
{
    // 当有新用户连接时，会执行TcpServer::newConnection回调
    m_acceptor->setNewConnectionCallback(std::bind(&TcpServer::newConnection,this,std::placeholders::_1,std::placeholders::_2));

}

TcpServer::~TcpServer()
{
    for(auto &item : m_connections)
    {
        TcpConnectionPtr conn(item.second);  // 这个局部的shared_ptr对象出右括号，可以自动释放new出来的TcpConnectionPtr的对象资源了
        item.second.reset();

        // 销毁连接
        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed,conn));
    }
}

void TcpServer::setThreadNum(int numThreads)
{
    m_threadPool->setThreadNum(numThreads);
}

void TcpServer::start()
{
    if(m_started ++ == 0) //防止一个对象被start多次
    {
        m_threadPool->start(m_threadInitCallback);   // 启动底层线程池
        m_loop->runInLoop(std::bind(&Acceptor::listen,m_acceptor.get()));
    }
}

// 根据轮询算法选择一个subloop，唤醒subloop，把当前connfd封装成channel分发给subloop
// 当有一个新客户端的连接，acceptor会执行这个回调操作

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    // 轮询算法选择一个subloop 来管理channel
    EventLoop *loop = m_threadPool->getNextLoop();
    char buf[64] = {0};
    snprintf(buf,sizeof(buf),"-%s#%d",m_ipPort.c_str(),m_nextConnId);
    ++m_nextConnId;
    std::string connName = m_name + buf;

    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s\n",m_name.c_str(),connName.c_str(),peerAddr.toIpPort().c_str());

    // 通过sockfd获取其绑定的本机的ip地址和端口信息
    sockaddr_in local;
    ::bzero(&local,sizeof(local));
    socklen_t addrlen = sizeof(local);
    
    if(::getsockname(sockfd,(sockaddr *)&local,&addrlen) < 0)
    {
        LOG_ERROR("sockets::getlocaladdr");
    }

    InetAddress localAddr(local);

    // 根据连接成功的sockfd，创建TcpConnection连接对象
    TcpConnectionPtr conn(new TcpConnection(loop,connName,sockfd,localAddr,peerAddr));
    m_connections[connName] = conn;

    // 下面的回调都是用户设置给TcpServer，TcpServer设置给Tcponnection，在设置给channel，在设置给poller 通知channel调用回调
    conn->setConnectionCallback(m_connectionCallback);
    conn->setMessageCallback(m_messageCallback);
    conn->setWriteCompleteCallback(m_writeCompleteCallback);

    // 设置了如何关闭连接的回调
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection,this,std::placeholders::_1));

    // 直接调用connectEstablished方法
    loop->runInLoop(std::bind(&TcpConnection::connectEstablished,conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    m_loop->runInLoop(std::bind(&TcpServer::removeConnectionInLoop,this,conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s\n",m_name.c_str(),conn->name().c_str());

    m_connections.erase(conn->name());
    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed,conn));
}
