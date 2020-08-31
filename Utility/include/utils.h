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
#include <TH1F.h>
#include <TCanvas.h>
#include <TLatex.h>

#include <torch/torch.h>

#include <ChargedAnalysis/Utility/include/plotutil.h>

namespace Utils{
    //Function for splitting string delimited by white space
    template <typename T>
    std::vector<T> SplitString(const std::string& splitString, const std::string& delimeter);

    //Joins vector of strings together
    std::string Join(const std::string& delimeter, const std::vector<std::string> strings);

    //Replaces symbol in string like python string format
    template <typename T>
    std::string Format(const std::string& label, const std::string& initial, const T& replace, const bool& ignoreMissing=false);

    //Merge two vectors
    template <typename T>
    std::vector<T> Merge(const std::vector<T>& vec1, const std::vector<T>& vec2);

    //Find string in vector
    template <typename T>
    int Find(const std::string& string, const T& itemToFind);

    //Find string in vector
    template <typename T>
    T* CheckNull(T* ptr, const char* function = __builtin_FUNCTION(), const char* file = __builtin_FILE(), int line = __builtin_LINE()){
        if(ptr != nullptr) return ptr;
        else{
            throw std::runtime_error("Null pointer returned: " + std::string(function) + " method in " + std::string(file) + ":" + std::to_string(line));
        } 
    };

    //Find string in vector
    template <typename T>
    int Find(const std::string& string, const T& itemToFind);

    //Check if zero, if yes, return 1.
    float CheckZero(const float& input);

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

    //Send file to dCache with symlinking
    void CopyToCache(const std::string inFile, const std::string outPath);

    unsigned int BitCount(unsigned int num);

    //Return TGraph with ROC curve
    TGraph* GetROC(const std::vector<float>& pred, const std::vector<int>& target, const int& nPoints = 200);
    void DrawScore(const torch::Tensor pred, const torch::Tensor truth, const std::string& scorePath);
};

#endif
