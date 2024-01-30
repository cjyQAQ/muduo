#include "Socket.h"
#include <unistd.h>
#include "Logger.h"
#include <sys/types.h>
#include <strings.h>
#include <sys/socket.h>
#include "InetAddress.h"
#include <netinet/tcp.h>
Socket::~Socket()
{
    close(m_sockfd);
}

void Socket::bindAddress(const InetAddress &localaddr)
{
    if(0 != ::bind(m_sockfd,(struct sockaddr *)localaddr.getSockAddr(),sizeof(struct sockaddr_in)))
    {
        LOG_FATAL("bind sockfd:%d fail\n",m_sockfd);
    }
}

void Socket::listen()
{
    if(1 != ::listen(m_sockfd,1024))
    {
        LOG_FATAL("listen sockfd:%d fail\n",m_sockfd);
    }
}

int Socket::accept(InetAddress *peeraddr)
{
    struct sockaddr_in addr;
    socklen_t len;
    bzero(&addr,sizeof(addr));
    int clientfd = ::accept(m_sockfd,(sockaddr *)&addr,&len);
    if(clientfd >= 0)
    {
        peeraddr->setSockAddr(addr);
    }
    return clientfd;
}

void Socket::shutdownWrite()
{
    if(::shutdown(m_sockfd,SHUT_WR) < 0)
    {
        LOG_ERROR("shutdown error\n");
    }
}

void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(m_sockfd,IPPROTO_TCP,TCP_NODELAY,&optval,sizeof(optval));
}

void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(m_sockfd,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));
}

void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(m_sockfd,SOL_SOCKET,SO_REUSEPORT,&optval,sizeof(optval));
}

void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(m_sockfd,SOL_SOCKET,SO_KEEPALIVE,&optval,sizeof(optval));
}
