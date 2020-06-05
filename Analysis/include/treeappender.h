#ifndef TREEAPPENDER_H
#define TREEAPPENDER_H

#include <torch/torch.h>

#include <string>
#include <vector>
#include <map>

#include <TFile.h>
#include <TTree.h>
#include <TBranch.h>

#include <ChargedAnalysis/Utility/include/utils.h>
#include <ChargedAnalysis/Utility/include/frame.h>
#include <ChargedAnalysis/Analysis/include/treefunction.h>
#include <ChargedAnalysis/Analysis/include/treeparser.h>
#include <ChargedAnalysis/Network/include/bdt.h>
#include <ChargedAnalysis/Network/include/dnnmodel.h>
#include <ChargedAnalysis/Network/include/htagger.h>
#include <ChargedAnalysis/Network/include/htagdataset.h>

class TreeAppender{
    private:
        std::string oldFile, oldTree, newFile; 
        std::vector<std::string> branchNames;
        std::string dCacheDir;

        std::vector<float> HScore(const int& FJIndex, const int& length);
        std::map<int, std::vector<float>> DNNScore(const std::vector<int>& masses, TTree* oldT);
        std::map<int, std::vector<float>> BDTScore(const std::vector<int>& masses, TTree* oldT);
        
    public:
        TreeAppender();
        TreeAppender(const std::string& oldFile, const std::string& oldTree, const std::string& newFile, const std::vector<std::string>& branchNames, const std::string& dCacheDir = "");

        void Append();
};

#endif
