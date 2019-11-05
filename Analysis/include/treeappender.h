#ifndef TREEAPPENDER_H
#define TREEAPPENDER_H

#include <string>
#include <vector>

#include <TFile.h>
#include <TTree.h>
#include <TBranch.h>
#include <TMath.h>
#include <Math/GenVector/LorentzVector.h>
#include <Math/GenVector/VectorUtil.h>
#include <Math/Vector3Dfwd.h>
#include <Math/Vector4Dfwd.h>

#include <ChargedAnalysis/Analysis/include/utils.h>
#include <ChargedAnalysis/Network/include/htagger.h>

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
        
    public:
        TreeAppender();
        TreeAppender(const std::string& oldFile, const std::string& oldTree, const std::string& newFile, const std::vector<std::string>& branchNames, const int& entryStart, const int& entryEnd);

        void Append();
};

#endif
