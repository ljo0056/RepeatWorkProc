#ifndef INNERTHREAD_H
#define INNERTHREAD_H

///  @author  Lee Jong Oh
///  @class   InnerThread
///  @brief   C++11 STL 의 std::thread Wapper class 이다.

#include <thread>
#include <chrono>

class InnerThread
{
private:
    std::thread m_thread;
    std::string m_thread_name;

protected:
    virtual bool StartThread();
    virtual void JoinThread();
    virtual void ThreadLoop() = 0;

public:
    InnerThread() = default;
    virtual ~InnerThread();

    void SaveThreadName(const std::string& thread_name);
    void Sleep(int ms);
};

#endif // INNERTHREAD_H


// Sample code
#if 0

class WorkThread : public InnerThread
{
public:
    bool m_running = false;

    WorkThread()
    {
        SaveThreadName("Work Thread");
    }
    virtual ~WorkThread()
    {
    }

    int  Activate()
    {
        InnerThread::StartThread();
        return 0;
    }
    int  Deactivate()
    {
        m_running = false;
        InnerThread::JoinThread();
        return 0;
    }

protected:
    virtual void ThreadLoop()
    {
        m_running = true;
        while (true)
        {
            if (false == m_running)
                break;

            // Do Something...
        }
    }
};


void main()
{
    WorkThread work;

    work.Activate();
    // Do Something...
    getchar();

    work.Deactivate();
}

#endif