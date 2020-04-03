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
#include <ChargedAnalysis/Analysis/include/treereader.h>
#include <ChargedAnalysis/Analysis/include/treefunction.h>
#include <ChargedAnalysis/Network/include/bdt.h>
#include <ChargedAnalysis/Network/include/dnnmodel.h>
#include <ChargedAnalysis/Network/include/htagger.h>
#include <ChargedAnalysis/Network/include/htagdataset.h>

#include <torch/torch.h>

class TreeAppender{
    private:
        std::string oldFile;
        std::string oldTree;  
        std::string newFile; 
        std::vector<std::string> branchNames;
        int entryStart; 
        int entryEnd;

        std::vector<float> HScore(const int& FJIndex);
        std::map<int, std::vector<float>> DNNScore(const std::vector<int>& masses, TTree* oldT);
        std::map<int, std::vector<float>> BDTScore(const std::vector<int>& masses, TTree* oldT);
        
    public:
        TreeAppender();
        TreeAppender(const std::string& oldFile, const std::string& oldTree, const std::string& newFile, const std::vector<std::string>& branchNames, const int& entryStart, const int& entryEnd);

        void Append();
};

#endif
