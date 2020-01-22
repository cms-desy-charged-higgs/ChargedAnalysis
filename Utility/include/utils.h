#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>
#include <sstream>
#include <iterator>
#include <iostream>
#include <chrono>
#include <algorithm>
#include <map>

#include <TGraph.h>
#include <torch/torch.h>

namespace Utils{
    //Function for splitting string delimited by white space
    std::vector<std::string> SplitString(const std::string& splitString, const std::string& delimeter);

    //Function for displaying progress
    void ProgressBar(const int& progress, const std::string& process);

    //Struct for measure executing time
    class RunTime{
        //Measure execution time
        private:
            std::chrono::steady_clock::time_point start;
            std::chrono::steady_clock::time_point end;

        public:
            RunTime();
            float Time();
    };

    //channel name to dir name
    std::string ChanPaths(const std::string& channel);

    //Find string in vector
    int FindInVec(const std::vector<std::string>& vect, const std::string& itemToFind);

    int Ratio(const float& num, const float& dem);

    //Return TGraph with ROC curve
    TGraph* GetROC(const torch::Tensor pred, const torch::Tensor target, const int& nPoints = 200);
};

#endif
