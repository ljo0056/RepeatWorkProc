#pragma once

///  @file    TimerLockerManager.h
///  @date    2019/07/15
///  @author  Lee Jong Oh
///  @brief   일정 시간 마다 Event signal 을 발생하는 객체를 생성하고 사용 한다.

#include <map>
#include <mutex>
#include <functional>

#include "Locker.h"

//////////////////////////////////////////////////////////////////////////
///  @class   CTimerLocker
///  @brief   일정 시간 마다 Event signal 을 수신 받으면 block 이 해제 되는 Timer Locket class

class CTimerLocker : public Locker
{
    friend class CTimerLockerManager;

private:
    using CallBackTimer = std::function<void(const CTimerLocker& locker)>;

    std::string     m_name;
    int             m_period = 0;
    int             m_period_count = 0;

    CallBackTimer   m_callback;

private:
    CTimerLocker(const std::string& name, int period);
    CTimerLocker(const std::string& name, int period, const CallBackTimer& callback);

    // [주의사항] Callback 함수에서는 오래 걸리는 작업을 수행하면 안된다.
    void SetCallback(const CallBackTimer& callback);

public:
    ~CTimerLocker();

    ///  @brief : 설정된 식별자 name 을 반환 한다.
    ///  @return : 설정된 식별자 name 을 리턴
    std::string GetName() const;

    ///  @brief : 설정된 타이머 시간(ms) 을 반환 한다.
    ///  @return : 타이머 시간 반환
    int  GetPeriod() const;

    ///  @brief : 설정된 FPS 을 반환 한다.
    ///  @return : FPS 반환
    int  GetFps() const;
};

//////////////////////////////////////////////////////////////////////////
///  @class   CTimerLockerManager
///  @brief   CTimerLocker 를 관리하고 Timer 를 통해서 Event 를 발생시켜 signal 을 전송해주는 class
///           singleton 으로 구현되어 있음

class CTimerLockerManager
{
private:
    class CTimerLockerList;
    using CallBackTimer = CTimerLocker::CallBackTimer;

    int    m_timer_min_resolution = 10;

    std::recursive_mutex    m_mutex_items;
    std::map<int, std::unique_ptr<CTimerLockerList>>    m_map_timers;   // key : period(ms), value : CTimerList

private:
    CTimerLockerManager();
    ~CTimerLockerManager();

    CTimerLockerList* GetList(int ms);
    int  GetDivisor(int ms);

    void CallbackTimer(int id, int ms);

public:
    ///  @brief      싱글턴 패턴으로 구현되어 있다.
    ///  @return     CTimerLockerManager 객체를 리턴 한다.
    static CTimerLockerManager& GetInstance()
    {
        static CTimerLockerManager manager;
        return manager;
    }

    ///  @brief : 타이머의 최소 해상도를 설정 한다.
    ///  @param ms[in] : 시간 설정 (millisecond)
    ///  @return : 없음
    void SetTimerMinResolution(int ms);
    ///  @brief : 타이머의 최소 해상도를 반환 한다.
    ///  @return : 타이머의 최소 해상도
    int  GetTimerMinResolution() const;

    ///  @brief : CTimerLocker 객체를 반환 한다. 주의 : 반환 받은 객체는 delete 를 하지 말자.
    ///  @param name[in] : CTimerLocker 를 식별해주는 이름
    ///  @param ms[in] : 시간 설정 (millisecond)
    ///  @param callback[in] : 설정된 시간마다 호출되는 callback 함수
    ///  @return : CTimerLocker 객체
    CTimerLocker* GetTimerLockerByTime(const std::string& name, int ms, const CallBackTimer& callback = CallBackTimer());

    ///  @brief : CTimerLocker 객체를 반환 한다. 주의 : 반환 받은 객체는 delete 를 하지 말자.
    ///  @param name[in] : CTimerLocker 를 식별해주는 이름
    ///  @param fps[in] : FPS 설정
    ///  @param callback[in] : 설정된 시간마다 호출되는 callback 함수
    ///  @return : CTimerLocker 객체
    CTimerLocker* GetTimerLockerByFps(const std::string& name, int fps, const CallBackTimer& callback = CallBackTimer());

    ///  @brief : GetTimerLockerByTime() 에서 사용한 CTimerLocker 객체를 반환하여 제거 한다.
    ///  @param timer_locker[in] : CTimerLocker 객체
    ///  @return : 성공 여부
    bool DeleteTimerLocker(CTimerLocker* timer_locker);

    ///  @brief : GetTimerLockerByTime() 에서 사용한 CTimerLocker 객체를 반환하여 제거 한다.
    ///  @param name[in] : CTimerLocker 객체를 식별해주는 name
    ///  @return : 성공 여부
    bool DeleteTimerLocker(const std::string& name);
};

// Sample code...
#if 0
#include <plog/Appenders/ColorConsoleAppender.h>

#include <chrono>
#include "TimerLockerManager.h"

void func_locker(CTimerLocker* locker, bool& stop)
{
    typedef std::chrono::high_resolution_clock::time_point   chrono_tp;
    typedef std::chrono::duration<double, std::milli>        chrono_duration_milli;

    chrono_tp fps_check = std::chrono::high_resolution_clock::now();
    int count = 0;
    int check_ms = 1000;

    LOGI << "Locker Start : " << locker->GetName();
    while (false == stop)
    {
        locker->Wait();
        count++;

        chrono_tp end = std::chrono::high_resolution_clock::now();
        chrono_duration_milli elapsed = end - fps_check;

        // 1초에 찍혀야할 카운트를 계산해서 그 시간을 출력 한다.
        int check_count = 1000 / locker->GetPeriod();   // 1초에 체크해야할 카운트수 계산
        if (check_count == count)
        {
            LOGI << "Locker recvive signal : " << locker->GetName() << " time : " << elapsed.count();
            fps_check = std::chrono::high_resolution_clock::now();
            count = 0;
        }

        // 초당 몇번의 카운트가 찍혔는지 로그로 표시한다.
        //if (elapsed.count() >= check_ms)
        //{
        //    LOGI << "Locker recvive signal : " << locker->GetName() << " count : " << count;
        //
        //    fps_check = std::chrono::high_resolution_clock::now();
        //    count = 0;
        //}
    }
    LOGI << "Locker End : " << locker->GetName();
}

int main()
{
    static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
    plog::init(plog::verbose, &consoleAppender);

    CTimerLockerManager& manager = CTimerLockerManager::GetInstance();

    CTimerLocker* locker1 = manager.GetTimerLockerByTime("timer_100ms", 100);
    CTimerLocker* locker2 = manager.GetTimerLockerByFps("timer_30fps", 30);

    bool stop = false;
    std::thread thread1 = std::thread([locker1, &stop]()
    {
        func_locker(locker1, stop);
    });

    std::thread thread2 = std::thread([locker2, &stop]()
    {
        func_locker(locker2, stop);
    });

    getchar();

    stop = true;
    if (thread1.joinable())
        thread1.join();
    if (thread2.joinable())
        thread2.join();

    return 0;
}


#endif