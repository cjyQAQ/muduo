#include "TcpServer.h"
#include "Logger.h"



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
             m_nextConnId(1)
              
{
    // 当有新用户连接时，会执行TcpServer::newConnection回调
    m_acceptor->setNewConnectionCallback(std::bind(&TcpServer::newConnection,this,std::placeholders::_1,std::placeholders::_2));

}

TcpServer::~TcpServer()
{
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
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    EventLoop *loop = m_threadPool->getNextLoop();

}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
}
