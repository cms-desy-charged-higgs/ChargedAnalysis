#ifndef BDT_H
#define BDT_H

#include <string>
#include <map>

#include <TFile.h>
#include <TTree.h>
#include <TBranch.h>
#include <TROOT.h>

#include <TMVA/Factory.h>
#include <TMVA/DataLoader.h>
#include <TMVA/Reader.h>
#include <TMVA/Tools.h>
#include <TMVA/Config.h>

class BDT{
    private:
        std::string trainString;
        TMVA::Reader* reader = NULL;

    public:
        BDT(const int &nTrees = 500, const float &minNodeSize = 2.5, const float &learningRate = 0.5, const int &nCuts = 20, const int &treeDepth = 3, const int &dropOut = 2, const std::string &sepType = "GiniIndex");
        ~BDT();


        float Train(std::vector<std::string> &xParameters, std::string &treeDir, std::string &resultDir, std::string &signal, std::vector<std::string> &backgrounds, std::vector<std::string>& masses, const bool& optimize);
        std::vector<std::string> SetEvaluation(const std::string &bdtPath);
        float Evaluate(const std::vector<float> &paramValues);
};

#endif
