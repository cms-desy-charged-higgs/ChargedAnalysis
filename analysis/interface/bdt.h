#ifndef BDT_H
#define BDT_H

#include <string>
#include <map>

#include <TFile.h>
#include <TTree.h>
#include <TBranch.h>

#include <TMVA/Factory.h>
#include <TMVA/DataLoader.h>
#include <TMVA/Reader.h>
#include <TMVA/Tools.h>

class BDT{
    private:
        std::string trainString;
        TMVA::Reader* reader;

    public:
        BDT(const int &nTrees = 500, const float &minNodeSize = 2.5, const float &learningRate = 0.5, const int &nCuts = 20, const int &treeDepth = 3, const int &dropOut = 2, const std::string &sepType = "GiniIndex");


        float Train(std::vector<std::string> &xParameters, std::string &treeDir, std::string &resultDir, std::vector<std::string> &signals, std::vector<std::string> &backgrounds, std::string &evType);
        std::vector<std::string> SetEvaluation(const std::string &bdtPath);
        float Evaluate(const std::vector<float> &paramValues);
};

#endif
