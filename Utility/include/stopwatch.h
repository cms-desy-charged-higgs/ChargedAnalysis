#ifndef STOPWATCH_H
#define STOPWATCH_H

#include <chrono>
#include <vector>

class StopWatch{
    private:
        std::chrono::steady_clock::time_point start;
        std::vector<std::chrono::steady_clock::time_point> timeMarker;

    public:
        StopWatch();
        float Start();
        float GetTime();
        float SetTimeMark();
        float GetTimeMark(const int& i);
};

#endif
