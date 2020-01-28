#ifndef FRAME_H
#define FRAME_H

#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <functional>
#include <fstream>
#include <exception>

#include <ChargedAnalysis/Utility/include/utils.h>

class Frame{
    private:
        std::vector<std::string> labels;
        std::vector<std::vector<float>> columns;

        static bool DoSort(std::vector<float>& column1, std::vector<float>& column2, const int& index, const bool& ascending);

    public: 
        Frame();
        Frame(const std::vector<std::string>& initLabels);

        bool AddRow(const std::string& label, const std::vector<float>& row);
        bool AddColumn(const std::vector<float>& column); 
        void Sort(const std::string& label, const bool& ascending=false);
        void ReadCSV(const std::string& fileName);
        void WriteCSV(const std::string& fileName);
};

#endif
