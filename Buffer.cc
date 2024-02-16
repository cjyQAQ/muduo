#include "Buffer.h"
#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>



Buffer::Buffer(size_t initialsize)
    :m_buffer(kCheapPrepend + initialsize),
     m_readerIndex(kCheapPrepend),
     m_writerIndex(kCheapPrepend) 
{

}

Buffer::~Buffer()
{
}

// POLLER工作在LT模式 Buffer有大小，但是从fd中读数据的时候，却不知道tcp数据最终大小
ssize_t Buffer::readFd(int fd, int *saveErrno)
{
    char extrabuf[65536] = {0};
    struct iovec vec[2];
    const size_t writeable = writeableBytes();   // 底层缓冲区中剩余的可以写的空间
    vec[0].iov_base = begin() + m_writerIndex;
    vec[0].iov_len = writeable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    const int iovcnt = (writeable < sizeof(extrabuf)) ? 2 : 1;
    const ssize_t n = readv(fd,vec,iovcnt);
    if(n < 0)
    {
        *saveErrno = errno;
    }
    else if(n <= writeable)    // buffer 中够用
    {
        m_writerIndex += n;
    }
    else   // buffer不够用，extrabuf也有数据
    {
        m_writerIndex = m_buffer.size();
        append(extrabuf,n - writeable);
    }
    return n;
}

ssize_t Buffer::writeFd(int fd, int *saveErrno)
{
    ssize_t n = ::write(fd,peek(),readableBytes());
    if(n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}
