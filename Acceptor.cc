#include "Acceptor.h"
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include "Logger.h"
#include <errno.h>
#include <unistd.h> 
#include "InetAddress.h"

static int createNonbolcking()
{
    int sockfd = ::socket(AF_INET,SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,IPPROTO_TCP);
    if(sockfd < 0)
    {
        LOG_FATAL("%s:%s:%d listen socket create error:%d\n",__FILE__,__FUNCTION__,__LINE__,errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
                    :m_listenning(false),
                     m_loop(loop),
                     m_acceptSocket(createNonbolcking()),
                     m_acceptChannel(loop,m_acceptSocket.fd())
{
    m_acceptSocket.setReuseAddr(true);
    m_acceptSocket.setReusePort(true);
    m_acceptSocket.bindAddress(listenAddr);   // bind
    // 当有一个新用户来进行连接，会执行一个回调，将fd封装到channel中，然后分发到一个subloop中
    // baseloop监听到listenfd
    m_acceptChannel.setReadCallback(std::bind(&Acceptor::handleRead,this));
}

Acceptor::~Acceptor()
{
    m_acceptChannel.disableAll();
    m_acceptChannel.remove();
}

void Acceptor::listen()
{
    m_listenning = true;
    m_acceptSocket.listen();
    m_acceptChannel.enableReading();

}

// listenfd有事件发生了，有新用户连接了执行该函数
void Acceptor::handleRead()
{
    InetAddress peerAddr;
    int connfd = m_acceptSocket.accept(&peerAddr);
    if(connfd >= 0)
    {
        if(m_newConnectionCallback)
        {
            m_newConnectionCallback(connfd,peerAddr);   // 轮询找到subloop，唤醒，分发当前的新客户端的channel
        }
        else
        {
            ::close(connfd);
        }
    }
    else
    {
        LOG_ERROR("%s:%s:%d accept socket error:%d\n",__FILE__,__FUNCTION__,__LINE__,errno);
        if(errno == EMFILE)
        {
            LOG_ERROR("%s:%s:%d sockfd reached limit!\n",__FILE__,__FUNCTION__,__LINE__);
        }
    }
}
