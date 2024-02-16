#include <mymuduo/TcpServer.h>
#include <mymuduo/Logger.h>

#include <string>
class EchoServer
{
public:
    EchoServer(EventLoop *Loop,const InetAddress &addr,const std::string &name)
        : m_server(Loop,addr,name),
          m_loop(Loop)
    {

        // 注册回调函数
        m_server.setConnectionCallback(std::bind(&EchoServer::onConnection,this,std::placeholders::_1));

        m_server.setMessageCallback(std::bind(&EchoServer::onMessage,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
        // 设置loop线程个数
        m_server.setThreadNum(3);
    }

    void start()
    {
        m_server.start();
    }
private:
    // 连接建立或者断开的回调
    void onConnection(const TcpConnectionPtr &conn)
    {
        if(conn->connected())
        {
            LOG_INFO("conn UP : %s",conn->peerAddress().toIpPort().c_str());
        }
        else
        {
            LOG_INFO("conn Down : %s",conn->peerAddress().toIpPort().c_str());
        }
    }

    // 可读写事件回调
    void onMessage(const TcpConnectionPtr &conn,Buffer *buf,Timestamp time)
    {
        std::string msg = buf->retrieveAllAsString();
        conn->send(msg);
        conn->shutdown();  // 写端  epollhup => closecallback
    }

    EventLoop *m_loop;
    TcpServer m_server;
};

int main()
{
    EventLoop loop;
    InetAddress addr(8000);
    
    EchoServer server(&loop,addr,"EchoServer-01");  // Acceptor non-blocking listenfd create bind
    server.start(); // listen loopthread listenfd acceptchannel mainloop 
    loop.loop(); // 启动mainloop底层的poller
    return 0;
}