#include "Locker.h"

Locker::Locker()
{

}

Locker::~Locker()
{
    m_condition_variable.notify_all();
}

bool Locker::WaitProc()
{
    bool ret = false;
    if (m_lock_count > 0)
    {
        m_lock_count--;
        ret = true;
    }

    return ret; // true 를 return 하면 block 이 풀린다.
}

bool Locker::Wait()
{
    std::unique_lock<std::mutex> locker(m_mutex);
    m_condition_variable.wait(locker, [this]
    {
        return WaitProc();
    });

    return true;
}

bool Locker::Wait(int ms)
{
    std::unique_lock<std::mutex> locker(m_mutex);
    bool ret = m_condition_variable.wait_for(locker, std::chrono::milliseconds(ms), [this]
    {
        return WaitProc();
    });

    return ret;
}

void Locker::WakeUp(bool notity_all)
{
    {
        std::unique_lock<std::mutex> locker(m_mutex);
        m_lock_count = m_lock_max_count;
    }

    if (notity_all)
        m_condition_variable.notify_all();
    else
        m_condition_variable.notify_one();
}

void Locker::SetWakeUpCount(int count)
{
    m_lock_max_count = count;
}

int Locker::GetLockCount() const
{
    return m_lock_max_count;
}