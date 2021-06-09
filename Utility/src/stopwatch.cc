#include <ChargedAnalysis/Utility/include/stopwatch.h>

StopWatch::StopWatch(){}

float StopWatch::Start(){
    start = std::chrono::steady_clock::now();
    timeMarker.clear();

    return 0.;
}

float StopWatch::GetTime(){
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count();
}

float StopWatch::SetTimeMark(){
    timeMarker.push_back(std::chrono::steady_clock::now());

    return std::chrono::duration_cast<std::chrono::milliseconds>(timeMarker.back() - start).count()/1e3;
}

float StopWatch::GetTimeMark(const int& i){
    return std::chrono::duration_cast<std::chrono::milliseconds>(timeMarker.at(i) - start).count()/1e3;
}
