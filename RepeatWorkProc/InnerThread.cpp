#include "InnerThread.h"

//////////////////////////////////////////////////////////////////////////
// class InnerThread

InnerThread::~InnerThread()
{
    JoinThread();
}

bool InnerThread::StartThread()
{
    m_thread = std::thread([this]()
    {
        ThreadLoop();
    });

    return true;
}

void InnerThread::JoinThread()
{
    if (m_thread.joinable())
        m_thread.join();
}

void InnerThread::SaveThreadName(const std::string& thread_name)
{
    m_thread_name = thread_name;
}

void InnerThread::Sleep(int ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

