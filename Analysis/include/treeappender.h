#ifndef TREEAPPENDER_H
#define TREEAPPENDER_H

#include <torch/torch.h>

#include <string>
#include <vector>
#include <map>

#include <TFile.h>
#include <TTree.h>
#include <TBranch.h>

#include <ChargedAnalysis/Utility/include/extension.h>

class TreeAppender{
    private:
        std::string fileName, treeName; 
        std::vector<std::string> appendFunctions;
        std::string dCacheDir;
        
    public:
        TreeAppender();
        TreeAppender(const std::string& fileName, const std::string& treeName, const std::vector<std::string>& appendFunctions, const std::string& dCacheDir = "");

        void Append();
};

#endif
