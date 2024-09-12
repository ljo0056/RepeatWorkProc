#pragma once

//////////////////////////////////////////////////////////////////////////
///  @file    RepeatWorkProc.h
///  @date    2019/09/19
///  @author  Lee Jong Oh


#include <mutex>
#include <queue>
#include <map>
#include <functional>

#include "InnerThread.h"
#include "Locker.h"

class CTimerLocker;

//////////////////////////////////////////////////////////////////////////
///  @class   RepeatWorkProc
///  @brief   일정 주기 마다 등록된 콜백 함수를 반복적으로 호출해주는 모듈이다.

class RepeatWorkProc : public InnerThread
{
private:
    using RepeatWork = std::function<void()>;

    bool                            m_thread_running = false;
    Locker                          m_queue_repeat_event;
    std::recursive_mutex            m_queue_repeat_mutex;
    std::queue<int>                 m_queue_repeat_work;
    std::map<int, CTimerLocker*>    m_map_timer;
    std::map<int, RepeatWork>       m_map_work;

private:
    RepeatWorkProc();
    virtual ~RepeatWorkProc();

    virtual void ThreadLoop() override;

    std::string GetTimerName(int work_type) const;

public:
    ///  @brief      싱글턴 패턴으로 구현되어 있다.
    ///  @return     RepeatWorkProc 객체를 리턴 한다.
    static RepeatWorkProc& GetInstance()
    {
        static RepeatWorkProc manager;
        return manager;
    }

    ///  @brief      활성화, 비활성화 시킨다.
    ///  @return     성공 시에 0, 실패 시에 1이상 값을 리턴
    int  Activate();
    int  Deactivate();

    ///  @brief : 식별자를 통해 일정 주기마다 호출되는 콜백 함수를 등록 한다.
    ///  @param work_type[in] : Work 의 식별자
    ///  @param ms[in] : 시간을 설정 (millisecond)
    ///  @param work[in] : ms 시간 마다 호출되는 Work 콜백 함수
    ///  @return : 성공 시에 0, 실패 시에 1이상 값을 리턴
    int  AddWork(int work_type, int ms, const RepeatWork& work);

    ///  @brief : work_type 식별자를 통해 일정 주기마다 호출되는 콜백 함수를 제거 한다.
    ///  @param work_type[in] : AddWork() 에서 사용한 Work 의 식별자
    ///  @return : 성공 시에 0, 실패 시에 1이상 값을 리턴
    int  DeleteWork(int work_type);
};

int TestRepeatWorkProc();