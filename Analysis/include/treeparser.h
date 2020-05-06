#ifndef TREEPARSER
#define TREEPARSER

#include <TH1.h>

#include <ChargedAnalysis/Analysis/include/treefunction.h>

class TreeParser{
    public:
        TreeParser();

        void GetFunction(const std::string& parameter, TreeFunction& func);
        void GetParticle(const std::string& parameter, TreeFunction& func);
        void GetCut(const std::string& parameter, TreeFunction& func);
        void GetBinning(const std::string& parameter, TH1* hist);        
};

#endif


