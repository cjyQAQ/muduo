#include "Thread.h"
#include "CurrentThread.h"
#include <semaphore.h>

std::atomic_int32_t Thread::m_numCreated{0};


Thread::Thread(ThreadFunc func, const std::string &name)
                    :m_started(false),
                     m_joined(false),
                     m_tid(0),
                     m_func(std::move(func)),
                     m_name(name)
{
    setDefaultName();
}

Thread::~Thread()
{
    if(m_started && !m_joined)
    {
        m_thread->detach();            // 分离态
    }
}

void Thread::start()  // 一个thread对象记录的是一个线程的详细信息
{
    m_started = true;
    sem_t sem;
    sem_init(&sem,false,0);
    // 开启线程
    m_thread = std::shared_ptr<std::thread>(new std::thread([&](){
        // 获取线程的tid
        m_tid = CurrentThread::tid();
        sem_post(&sem);
        m_func();  // 开启一个新线程，专门执行该线程函数
    }));

    // 这里必须等上面创建的线程的tid值
    sem_wait(&sem);
}
void Thread::join()
{
    m_joined = true;
    m_thread->join(); 
}

void Thread::setDefaultName()
{
    int num = ++m_numCreated;
    if(m_name.empty())
    {
        char buf[32] = {0};
        snprintf(buf,sizeof(buf),"Thread%d",num);
        m_name = buf;
    }
}