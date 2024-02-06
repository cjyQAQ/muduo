# pragma once 

#include "noncopyable.h"
#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"

#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>
class TcpServer : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    enum Option
    {
        kNoReusePort,
        kReusePort
    };

    TcpServer(EventLoop *loop,const InetAddress &listenAddr,const std::string &name,Option option = kNoReusePort);
    ~TcpServer();

    void setThreadInitCallback(const ThreadInitCallback &cb) { m_threadInitCallback = std::move(cb); }
    void setConnectionCallback(const ConnectionCallback &cb) { m_connectionCallback = std::move(cb); }
    void setMessageCallback(const MessageCallback &cb) { m_messageCallback = std::move(cb); }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { m_writeCompleteCallback = std::move(cb); }
    

    // 设置底层subloop的个数
    void setThreadNum(int numThreads);

    // 开启监听
    void start();
private:
    void newConnection(int sockfd,const InetAddress &peerAddr);
    void removeConnection(const TcpConnectionPtr &conn);
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    using ConnectionMap = std::unordered_map<std::string,TcpConnectionPtr>;

    EventLoop *m_loop;
    const std::string m_ipPort;
    const std::string m_name;

    std::unique_ptr<Acceptor> m_acceptor;  // 运行在mainloop，任务就是监听新连接事件

    std::shared_ptr<EventLoopThreadPool> m_threadPool;   // one loop per thread
    
    ConnectionCallback m_connectionCallback;   // 有新连接时的回调
    MessageCallback m_messageCallback;    // 有读写消息时的回调
    WriteCompleteCallback m_writeCompleteCallback;   //消息发送完成后的回调

    ThreadInitCallback m_threadInitCallback;  // loop线程初始化后的回调
    std::atomic_int m_started;

    int m_nextConnId;
    ConnectionMap m_connections; // 保存所有连接
};
