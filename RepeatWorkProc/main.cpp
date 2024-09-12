// RepeatWorkProc.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "RepeatWorkProc.h"

int main()
{
    RepeatWorkProc& repeat_work = RepeatWorkProc::GetInstance();
    if (repeat_work.Activate())
        return 1;

    enum
    {
        TEST_WORK_1 = 0,
        TEST_WORK_2 = 1,
    };

    int count_func1 = 0;
    auto func1 = [&count_func1]()
        {
            RepeatWorkProc& repeat_work = RepeatWorkProc::GetInstance();

            count_func1++;
            printf("Work1 Count[%d]\n", count_func1);
            if (5 == count_func1)
                repeat_work.DeleteWork(TEST_WORK_1);
        };

    int count_func2 = 0;
    auto func2 = [&count_func2]()
        {
            RepeatWorkProc& repeat_work = RepeatWorkProc::GetInstance();

            count_func2++;
            printf("Work2 Count[%d]\n", count_func2);
            if (5 == count_func2)
                repeat_work.DeleteWork(TEST_WORK_2);
        };

    repeat_work.AddWork(TEST_WORK_1, 1000, func1);
    repeat_work.AddWork(TEST_WORK_2, 1500, func2);

    while (true)
    {
        if (count_func1 == 5 && count_func2 == 5)
            break;

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    repeat_work.Deactivate();
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
