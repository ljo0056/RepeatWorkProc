#pragma once

///  @author  Lee Jong Oh

#include <mutex>
#include <condition_variable>

/**
 * @brief The UniqueLock class
 * wrapper for mutex and unique_lock.
 * Windows api 의 class CEvent 와 같은 기능을 갖는 C++11 STL Wapper class 이다.
 */
class Locker 
{
private:
    int                         m_lock_count     = 0;
    int                         m_lock_max_count = 1;

    std::mutex                  m_mutex;
    std::condition_variable     m_condition_variable;

private:
    bool WaitProc();

public:
    Locker();
    ~Locker();

    bool Wait();
    bool Wait(int ms);
    void WakeUp(bool notity_all = false);

    // 설정된 count 만큼 WakeUp function 을 호출 해주어야 Wait function 의 Block 이 풀린다.
    void SetWakeUpCount(int count);
    int  GetLockCount() const;
};

