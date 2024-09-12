#include "TimerLockerManager.h"
#include "TimerEx.h"

#include <vector>
#include <cmath>
#include <basetsd.h>

//////////////////////////////////////////////////////////////////////////
// class CTimerLocker

CTimerLocker::CTimerLocker(const std::string& name, int period)
    : m_name(name)
    , m_period(period)
{
}

CTimerLocker::CTimerLocker(const std::string& name, int period, const CallBackTimer& callback)
    : m_name(name)
    , m_period(period)
    , m_callback(callback)
{
}

CTimerLocker::~CTimerLocker()
{
}

void CTimerLocker::SetCallback(const CallBackTimer& callback)
{
    m_callback = callback;
}

std::string CTimerLocker::GetName() const
{
    return m_name;
}

int CTimerLocker::GetPeriod() const
{
    return m_period;
}

int CTimerLocker::GetFps() const
{
    int fps = (int)std::round(1000 / m_period);
    return fps;
}

//////////////////////////////////////////////////////////////////////////
// class CTimerList

class CTimerLockerManager::CTimerLockerList
{
private:
    int                     m_period = 0;
    float                   m_fps    = 0;
    timer_ex::TimerIdEx     m_timer_id = -1;

    std::recursive_mutex    m_mutex_lockers;
    std::vector<std::unique_ptr<CTimerLocker>>  m_lockers;

public:
    CTimerLockerList(int ms)
        : m_period(ms)
    {
        m_fps = (float)std::round(1000 / float(ms));
    }

    ~CTimerLockerList()
    {
        Finalize();
    }

    int Initialize(std::function<void(timer_ex::TimerIdEx, void*)> func)
    {
        return timer_ex::CreateTimer(m_timer_id, m_period, func, IntToPtr(m_period));
    }

    void Finalize()
    {
        {
            std::lock_guard<std::recursive_mutex> lock(m_mutex_lockers);
            m_lockers.clear();
        }
        timer_ex::DeleteTimer(m_timer_id);
    }

    bool HasTimer()
    {
        return -1 == m_timer_id ? false : true;
    }

    int AddItem(CTimerLocker* item)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex_lockers);

        m_lockers.push_back(std::unique_ptr<CTimerLocker>(item));

        return 0;
    }

    bool DeleteItem(const std::string& name)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex_lockers);

        bool ret = false;

        auto it = m_lockers.begin();
        while (it != m_lockers.end())
        {
            std::unique_ptr<CTimerLocker>& ptr = *it;
            if (ptr->m_name == name)
            {
                ptr->WakeUp();
                m_lockers.erase(it);
                ret = true;
                break;
            }
            it++;
        }

        return ret;
    }

    CTimerLocker* GetItem(const std::string& name)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex_lockers);

        CTimerLocker* item = nullptr;
        auto it = m_lockers.begin();
        while (it != m_lockers.end())
        {
            std::unique_ptr<CTimerLocker>& ptr = *it;
            if (ptr->m_name == name)
            {
                item = ptr.get();
                break;
            }
            it++;
        }

        return item;
    }

    bool IsEmpty()
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex_lockers);
        return m_lockers.empty();
    }

    void SendEvent()
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex_lockers);

        if (m_lockers.empty())
            return;

        auto it = m_lockers.begin();
        while (it != m_lockers.end())
        {
            std::unique_ptr<CTimerLocker>& ptr = *it;

            ptr->m_period_count += m_period;
            if (ptr->m_period_count >= ptr->m_period)
            {
                ptr->WakeUp();
                ptr->m_period_count = 0;
                if (ptr->m_callback)
                    ptr->m_callback(*ptr);
            }
            it++;
        }
    }
};

//////////////////////////////////////////////////////////////////////////
// class CTimerLockerManager

CTimerLockerManager::CTimerLockerManager()
{
    timer_ex::InitializeTimer();
}

CTimerLockerManager::~CTimerLockerManager()
{
    m_map_timers.clear();
}

void CTimerLockerManager::SetTimerMinResolution(int ms)
{
    m_timer_min_resolution = ms;
}

int CTimerLockerManager::GetTimerMinResolution() const
{
    return m_timer_min_resolution;
}

void CTimerLockerManager::CallbackTimer(int id, int ms)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex_items);

    auto it_list = m_map_timers.find(ms);
    if (it_list == m_map_timers.end())
        return;

    std::unique_ptr<CTimerLockerList>& timer_list = it_list->second;
    timer_list->SendEvent();
};

CTimerLockerManager::CTimerLockerList* CTimerLockerManager::GetList(int ms)
{
    auto func = [this](timer_ex::TimerIdEx id, void* ptr) {
        int ms = PtrToInt(ptr);
        CallbackTimer((int)id, ms);
    };

    CTimerLockerList* item_list = nullptr;
    auto it_list = m_map_timers.find(ms);
    if (it_list == m_map_timers.end())
    {
        item_list = new CTimerLockerList(ms);
        if (item_list->Initialize(func))
        {
            delete item_list;
            return nullptr;
        }
        m_map_timers[ms] = std::unique_ptr<CTimerLockerList>(item_list);
    }
    else
    {
        item_list = it_list->second.get();
    }

    return item_list;
}

int CTimerLockerManager::GetDivisor(int ms)
{
    int ret = GetTimerMinResolution();
    for (int ii = GetTimerMinResolution(); ii <= ms; ii++)
    {
        if (0 == (ms % ii)) 
        {
            ret = ii;
            break;
        }
    }

    return ret;
}

CTimerLocker* CTimerLockerManager::GetTimerLockerByTime(const std::string& name, int ms, const CallBackTimer& callback)
{
    if (ms < GetTimerMinResolution())
        ms = GetTimerMinResolution();

    std::lock_guard<std::recursive_mutex> lock(m_mutex_items);

    DeleteTimerLocker(name);  // 기존 Item 이 있다면 제거 한다.

    CTimerLockerList* item_list = GetList(GetDivisor(ms));
    if (nullptr == item_list)
        return nullptr;
    if (false == item_list->HasTimer())
        return nullptr;

    CTimerLocker* item = new CTimerLocker(name, ms, callback);
    item_list->AddItem(item);

    return item;
}

CTimerLocker* CTimerLockerManager::GetTimerLockerByFps(const std::string& name, int fps, const CallBackTimer& callback)
{
    if (0 == fps)
        return nullptr;

    return GetTimerLockerByTime(name, (int)std::round(1000 / fps), callback);
}

bool CTimerLockerManager::DeleteTimerLocker(CTimerLocker* timer_locker)
{
    return DeleteTimerLocker(timer_locker->GetName());
}

bool CTimerLockerManager::DeleteTimerLocker(const std::string& name)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex_items);

    bool ret = false;

    auto it = m_map_timers.begin();
    while (it != m_map_timers.end())
    {
        CTimerLockerList* item_list = it->second.get();
        if (item_list->DeleteItem(name))
        {
            if (item_list->IsEmpty())   // 모든 아이템을 제거 하면 존재해야할 필요가 없기 때문에 삭제한다.
            {
                item_list->Finalize();
                m_map_timers.erase(it);
            }

            ret = true;
            break;
        }

        it++;
    }

    return ret;
}