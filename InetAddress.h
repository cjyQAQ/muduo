#pragma once
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

class InetAddress
{
private:
    sockaddr_in m_addr; 
    
public:
    // InetAddress(/* args */);
    explicit InetAddress(uint16_t port = 0,std::string ip = "127.0.0.1");
    explicit InetAddress(const sockaddr_in &addr):m_addr(addr){}


    std::string toIP() const;
    std::string toIpPort() const;
    uint16_t toPort() const;
    const sockaddr_in *getSockAddr() const {return &m_addr;}
    void setSockAddr(const sockaddr_in &addr) { m_addr = addr; }
    ~InetAddress();
};