#ifndef TREESLIMMER
#define TREESLIMMER

#include <string>
#include <vector>

#include <TFile.h>
#include <TTree.h>

#include <ChargedAnalysis/Analysis/include/treefunction.h>
#include <ChargedAnalysis/Analysis/include/treereader.h>

class TreeSlimmer{
    private:
        const std::string inputFile;
        const std::string inputChannel;
    
    public:
        TreeSlimmer(const std::string& inputFile, const std::string& inputChannel);
    
        void DoSlim(const std::string outputFile, const std::string outChannel, const std::vector<std::string>& cuts);

};

#endif
