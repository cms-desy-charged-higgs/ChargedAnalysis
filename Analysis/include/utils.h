#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>
#include <sstream>
#include <iterator>
#include <iostream>
#include <chrono>

namespace Utils{
    //Function for splitting string delimited by white space
    std::vector<std::string> SplitString(const std::string& splitString);

    //Function for displaying progress
    void ProgressBar(const int& progress, const std::string& process);

    //Struct for measure executing time
    struct RunTime{
        //Measure execution time
        std::chrono::steady_clock::time_point start;
        std::chrono::steady_clock::time_point end;

        RunTime();
        float Time();
    };
};

#endif
