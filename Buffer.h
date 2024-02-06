#pragma once

#include "noncopyable.h"
#include <vector>
#include <string>

class Buffer : noncopyable
{
public:
    explicit Buffer(size_t initialsize = kInitialSize);
    ~Buffer();
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    size_t readableBytes() const { return m_writerIndex - m_readerIndex; }
    size_t writeableBytes() const { return m_buffer.size() - m_writerIndex; }
    size_t prependableBytes() const { return m_readerIndex; }

    const char *peek() const { return begin() + m_readerIndex; } //返回缓冲区中可读数据的起始地址
    void retrieve(size_t len) 
    {
        if(len < readableBytes())
        {
            m_readerIndex += len;
        }
        else
        {
            retrieveAll();
        }
    }
    void retrieveAll() 
    {
        m_readerIndex = kCheapPrepend;
        m_writerIndex = kCheapPrepend;
    }
    // 把onMessage函数上报的Buffer数据转成string类型的数据返回
    std::string retrieveAllAsString() { return retrieveAsString(readableBytes()); } // 应用可读取的数据长度
    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(),len);
        retrieve(len); //对缓冲区进行复位操作
        return result;
    }

    void ensureWritebaleBytes(size_t len)
    {
        if(writeableBytes() < len)
        {
            makeSpace(len);  // 扩容函数
        }
    }

    // 把【data，data+len】内存上的数据添加到writebale缓冲区中
    void append(const char * data,size_t len)
    {
        ensureWritebaleBytes(len);
        std::copy(data,data+len,beginWrite());
        m_writerIndex += len;
    }

    char *beginWrite()
    {
        return begin() + m_writerIndex;
    }

    const char *beginWrite() const
    {
        return begin() + m_writerIndex;
    } 

    // 从fd中读数据
    ssize_t readFd(int fd,int *saveErrno);
private:
    void makeSpace(size_t len)
    {
        if(writeableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            m_buffer.resize((m_writerIndex + len));
        }
        else
        {
            size_t readable = readableBytes();
            std::copy(begin() + m_readerIndex,begin() + m_writerIndex,begin() + kCheapPrepend);
            m_readerIndex = kCheapPrepend;
            m_writerIndex = m_readerIndex + readable;
        }
    }
    char *begin() { return &*m_buffer.begin(); }  // 数组起始地址
    const char *begin() const { return &*m_buffer.begin(); }
    std::vector<char> m_buffer;
    size_t m_readerIndex;
    size_t m_writerIndex;
};
