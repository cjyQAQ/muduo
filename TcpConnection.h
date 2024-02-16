#pragma once
#include "noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"


#include <memory>
#include <string>
#include <atomic>

class Socket;
class Channel;
class EventLoop;


// TcpServer 通过Acceptor 有一个新用户连接，通过accept拿到connfd，再通过Tcpconnection设置回调，打包给channel，channel又注册到poller上，当poller监听到事件后就执行回调
class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop *loop,const std::string &name,int sockfd,const InetAddress &localAddr,const InetAddress &peerAddr);
    ~TcpConnection();

    EventLoop *getLoop() const { return m_loop; }
    const std::string &name() const { return m_name; }
    const InetAddress &localAddress() const { return m_localAddr; }
    const InetAddress &peerAddress() const { return m_peerAddr; }

    bool connected() const { return m_state == kConnected; }
    
    // 发送数据
    void send(const std::string &buf);
    //关闭连接
    void shutdown();

    void setConnectionCallback(const ConnectionCallback &cb) { m_connectionCallback = std::move(cb); }
    void setMessageCallback(const MessageCallback &cb) { m_messageCallback = std::move(cb); }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { m_writeCompleteCallback = std::move(cb); }
    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb,size_t highWaterMark ) { m_highWaterMarkCallback = std::move(cb); m_highWaterMark = highWaterMark; }
    void setCloseCallback(const CloseCallback &cb) { m_closeCallback = std::move(cb); }
    
    // 连接建立
    void connectEstablished();
    // 连接销毁
    void connectDestroyed();
private:
    enum StateE
    {
        kDisconnected,
        kConnecting,
        kConnected,
        kDisconnecting
    };
    void handleRead(Timestamp recvtime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void *data,size_t len);
    void shutdownInLoop();

    void setState(StateE state) { m_state = state; }
    EventLoop *m_loop;   // subloop，tcpconnection都是在subloop中管理的
    const std::string m_name;

    std::atomic_int m_state;
    bool m_reading;

    std::unique_ptr<Socket> m_socket;
    std::unique_ptr<Channel> m_channel;

    const InetAddress m_localAddr;
    const InetAddress m_peerAddr;

    ConnectionCallback m_connectionCallback;   // 有新连接时的回调
    MessageCallback m_messageCallback;    // 有读写消息时的回调
    WriteCompleteCallback m_writeCompleteCallback;   //消息发送完成后的回调
    CloseCallback m_closeCallback;
    HighWaterMarkCallback m_highWaterMarkCallback;

    size_t m_highWaterMark;
    
    Buffer m_inputBuffer;   // 接收数据
    Buffer m_outputBuffer;  // 发送数据
};

