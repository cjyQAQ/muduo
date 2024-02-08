#include "TcpConnection.h"
#include "Logger.h"
#include "EventLoop.h"
#include "Channel.h"
#include "Socket.h"


#include <sys/types.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/tcp.h>


static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    if(loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d TcpConnection Loop is null\n",__FILE__,__FUNCTION__,__LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop, const std::string &name, int sockfd, const InetAddress &localAddr, InetAddress &peerAddr)
                :m_loop(CheckLoopNotNull(loop)),
                 m_name(name),
                 m_state(kConnecting),
                 m_reading(true),
                 m_socket(new Socket(sockfd)),
                 m_channel(new Channel(loop,sockfd)),
                 m_localAddr(localAddr),
                 m_peerAddr(peerAddr),
                 m_highWaterMark(64*1024*1024)   // 64M
{
    // 给channel设置相应的回调函数
    m_channel->setReadCallback(std::bind(&TcpConnection::handleRead,this,std::placeholders::_1));
    m_channel->setWriteCallback(std::bind(&TcpConnection::handleWrite,this));
    m_channel->setCloseCallback(std::bind(&TcpConnection::handleClose,this));
    m_channel->setErrorCallback(std::bind(&TcpConnection::handleError,this));

    LOG_INFO("TcpConnection::ctor[%s] at fd = %d\n",m_name.c_str(),sockfd);
    m_socket->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s] at fd = %d state = %d\n",m_name.c_str(),m_channel->fd(),(int)m_state);
}

void TcpConnection::handleRead(Timestamp recvtime)
{
    int savedError = 0;
    ssize_t n = m_inputBuffer.readFd(m_channel->fd(),&savedError);
    if(n > 0)
    {
        // 已经建立连接的用户有可读事件发生了，调用用户传入的回调操作onMessage
        m_messageCallback(shared_from_this(),&m_inputBuffer,recvtime);
    }
    else if(n == 0)
    {
        handleClose();
    }
    else
    {
        errno = savedError;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if(m_channel->isWriting())
    {
        int savedError = 0;
        ssize_t n = m_outputBuffer.writeFd(m_channel->fd(),&savedError);
        if(n > 0)
        {
            m_outputBuffer.retrieve(n);
            if(m_outputBuffer.readableBytes() == 0)
            {
                m_channel->disableWriting();
                if(m_writeCompleteCallback)
                {
                    // 唤醒loop对应的thread执行回调
                    m_loop->queueInLoop(std::bind(m_writeCompleteCallback,shared_from_this()));
                }
                if(m_state == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite")
        }
    }
    else
    {
        LOG_ERROR("TcpConnection::handleWrite fd = %d is down, no more writing\n",m_channel->fd());
    }
    
}

void TcpConnection::handleClose()
{
    LOG_INFO("TcpConnection fd = %d state = %d\n",m_channel->fd(),(int)m_state);
    setState(kDisconnected);
    m_channel->disableAll();

    TcpConnectionPtr guard(shared_from_this());
    m_connectionCallback(guard);   // 执行连接关闭的回调
    m_closeCallback(guard);  // 关闭连接的回调
} 

void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof(optval);
    int err = 0;
    if(::getsockopt(m_channel->fd(),SOL_SOCKET,SO_ERROR,&optval,&optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d\n",m_name.c_str(),err);
    
}
