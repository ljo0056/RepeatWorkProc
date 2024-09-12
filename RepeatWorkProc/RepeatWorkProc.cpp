#include "RepeatWorkProc.h"
#include "TimerLockerManager.h"

RepeatWorkProc::RepeatWorkProc()
{
    InnerThread::SaveThreadName("RepeatWorkProc");
}

RepeatWorkProc::~RepeatWorkProc()
{

}

int RepeatWorkProc::Activate()
{
    if (m_thread_running)
        return 1;

    InnerThread::StartThread();

    return 0;
}

int RepeatWorkProc::Deactivate()
{
    CTimerLockerManager& timer_manager = CTimerLockerManager::GetInstance();
    for (auto it_timer = m_map_timer.begin() ; it_timer != m_map_timer.end() ; it_timer++)
        timer_manager.DeleteTimerLocker(it_timer->second);

    m_map_timer.clear();
    m_map_work.clear();

    m_thread_running = false;
    m_queue_repeat_event.WakeUp();
    InnerThread::JoinThread();

    while (m_queue_repeat_work.size())
        m_queue_repeat_work.pop();

    return 0;
}

std::string RepeatWorkProc::GetTimerName(int work_type) const
{
    return std::string("RepeatTimer_" + std::to_string(work_type));
}

int RepeatWorkProc::AddWork(int work_type, int ms, const RepeatWork& work)
{
    auto it = m_map_work.find(work_type);
    if (it != m_map_work.end())
        return 1;

    CTimerLockerManager& timer_manager = CTimerLockerManager::GetInstance();

    {
        std::lock_guard<std::recursive_mutex> lock(m_queue_repeat_mutex);
        m_map_work[work_type] = work;
    }

    std::string timer_name = GetTimerName(work_type);

    auto func = [this, work_type](const CTimerLocker& locker) {
        m_queue_repeat_work.push(work_type);
        m_queue_repeat_event.WakeUp();
    };

    CTimerLocker* timer = timer_manager.GetTimerLockerByTime(timer_name, ms, func);

    m_map_timer[work_type] = timer;

    return 0;
}

int RepeatWorkProc::DeleteWork(int work_type)
{
    auto it_work = m_map_work.find(work_type);
    if (it_work == m_map_work.end())
        return 1;

    auto it_timer = m_map_timer.find(work_type);
    if (it_timer == m_map_timer.end())
        return 2;

    CTimerLockerManager& timer_manager = CTimerLockerManager::GetInstance();
    timer_manager.DeleteTimerLocker(it_timer->second);
    m_map_timer.erase(it_timer);

    {
        std::lock_guard<std::recursive_mutex> lock(m_queue_repeat_mutex);
        m_map_work.erase(it_work);
    }

    return 0;
}

void RepeatWorkProc::ThreadLoop()
{
    m_thread_running = true;
    while (true)
    {
        m_queue_repeat_event.Wait();
        if (false == m_thread_running)
            break;

        std::lock_guard<std::recursive_mutex> lock(m_queue_repeat_mutex);
        while (m_queue_repeat_work.size())
        {
            int work_type = m_queue_repeat_work.front();
            m_queue_repeat_work.pop();

            auto it = m_map_work.find(work_type);
            if (it == m_map_work.end())
                continue;

            RepeatWork& func = it->second;
            if (func)
                func();
        }
    }
}

int TestRepeatWorkProc()
{
    RepeatWorkProc& repeat_work = RepeatWorkProc::GetInstance();
    if (repeat_work.Activate())
        return 1;

    enum
    {
        TEST_WORK_1 = 0,
    };

    int count_func1 = 0;
    auto func1 = [&count_func1]()
    {
        RepeatWorkProc& repeat_work = RepeatWorkProc::GetInstance();

        count_func1++;
        printf("Work Count[%d]\n", count_func1);
        if (5 == count_func1)
            repeat_work.DeleteWork(TEST_WORK_1);
    };

    repeat_work.AddWork(TEST_WORK_1, 1000 * 1, func1);

    while (5 != count_func1)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    repeat_work.Deactivate();

    return 0;
}