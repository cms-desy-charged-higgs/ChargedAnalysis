#ifndef TREESLIMMER
#define TREESLIMMER

#include <string>
#include <vector>

#include <TFile.h>
#include <TTree.h>
#include <TEntryList.h>

#include <ChargedAnalysis/Analysis/include/treefunction.h>
#include <ChargedAnalysis/Analysis/include/treereader.h>
#include <ChargedAnalysis/Utility/include/utils.h>

class TreeSlimmer{
    private:
        const std::string inputFile;
        const std::string inputChannel;
    
    public:
        TreeSlimmer(const std::string& inputFile, const std::string& inputChannel);
    
        void DoSlim(const std::string outputFile, const std::string outChannel, const std::vector<std::string>& cuts, const int& start, const int& end);
};

#endif
