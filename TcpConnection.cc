#include "TcpConnection.h"
#include "Logger.h"
#include "EventLoop.h"
#include "Channel.h"
#include "Socket.h"


#include <sys/types.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <string>


static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    if(loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d TcpConnection Loop is null\n",__FILE__,__FUNCTION__,__LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop, const std::string &name, int sockfd, const InetAddress &localAddr, const InetAddress &peerAddr)
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

void TcpConnection::send(const std::string &buf)
{
    if(m_state == kConnected)
    {
        if(m_loop->isInLoopThread())
        {
            sendInLoop(buf.c_str(),buf.size());
        }
        else
        {
            m_loop->runInLoop(std::bind(&TcpConnection::sendInLoop,this,buf.c_str(),buf.size()));
        }
    }

}

void TcpConnection::shutdown()
{
    if(m_state == kConnected)
    {
        setState(kDisconnecting);
        m_loop->runInLoop(std::bind(&TcpConnection::shutdownInLoop,this));
    }
}

void TcpConnection::connectEstablished()
{
    setState(kConnected);
    m_channel->tie(shared_from_this());
    m_channel->enableReading();   // 注册epollin事件
    m_connectionCallback(shared_from_this()); // 连接建立
}

void TcpConnection::connectDestroyed()
{
    if(m_state == kConnected)
    {
        setState(kDisconnected);
        m_channel->disableAll();   // 把channel的所有感兴趣的事件从poller中del掉
        m_connectionCallback(shared_from_this());
    }
    m_channel->remove(); // 把channel从poller中del掉
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


// 底层的poller通知channel调用closeCallback方法 然后 会调用TcpConnection::handleClose()方法
void TcpConnection::handleClose()
{
    LOG_INFO("TcpConnection fd = %d state = %d\n",m_channel->fd(),(int)m_state);
    setState(kDisconnected);
    m_channel->disableAll();

    TcpConnectionPtr guard(shared_from_this());
    m_connectionCallback(guard);   // 执行连接关闭的回调
    m_closeCallback(guard);  // 关闭连接的回调 执行TcpServer::removeConnection的回调方法
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

// 发送数据 应用写的快，内核发送慢，需要把数据写入缓冲区并且设置水位控制速度
void TcpConnection::sendInLoop(const void *data, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool fault = false;

    // 之前调用过shutdown不能再进行发送了
    if(m_state == kDisconnected)
    {
        LOG_ERROR("disconnected,give up writing!");
        return ;
    }

    // channel 第一次写数据，而且缓冲区没有待发送数据
    if(!m_channel->isWriting() && m_outputBuffer.readableBytes() == 0)
    {
        nwrote = ::write(m_channel->fd(),data,len);
        if(nwrote >=0 )
        {
            remaining = len - nwrote;
            if(remaining == 0 && m_writeCompleteCallback)
            {
                // 数据全部发送完成就不用给channel设置epollout事件
                m_loop->queueInLoop(std::bind(m_writeCompleteCallback,shared_from_this()));
            }
        }
        else
        {
            nwrote = 0;
            if(errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::sendInLoop");
                if(errno == EPIPE || errno == ECONNRESET)  // SIGPIPE  SIGRESET
                {
                    fault = true;
                }
            }
        }
    }

    if(!fault && remaining > 0)   // 并没有一次性发送出去，剩余数据需要保存在缓冲区中，然后给channel注册epollout事件，poller发现tcp发送缓冲区有空间，会通知相应的channel，调用writecallback回调方法
    {
        // 目前发送缓冲区剩余的待发送数据的长度
        size_t oldLen = m_outputBuffer.readableBytes();
        if(oldLen + remaining >= m_highWaterMark && oldLen < m_highWaterMark && m_highWaterMarkCallback)
        {
            m_loop->queueInLoop(std::bind(m_highWaterMarkCallback,shared_from_this(),oldLen + remaining));
        }
        m_outputBuffer.append((char *)data + nwrote,remaining);
        if(!m_channel->isWriting())
        {
            m_channel->enableWriting();
        }
    }
}

void TcpConnection::shutdownInLoop()
{
    if(!m_channel->isWriting())  // 说明已经全部发送完成
    {
        m_socket->shutdownWrite();  // 关闭写端
    }
}
