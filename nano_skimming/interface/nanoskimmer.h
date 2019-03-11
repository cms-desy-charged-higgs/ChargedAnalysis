#include <ChargedHiggs/nano_skimming/interface/baseanalyzer.h>

#include <vector>
#include <string>
#include <chrono>
#include <memory>

#include <TFile.h>
#include <TTree.h>
#include <TTreeReader.h>

class NanoSkimmer{
    private:
        //Measure execution time
        std::chrono::steady_clock::time_point start;
        std::chrono::steady_clock::time_point end;

        //Input
        std::string inFile;
        bool isData;

        //Map with vector with analyzers
        std::map<std::string, std::vector<std::shared_ptr<BaseAnalyzer>>> analyzerMap;

        //Vector with wished analyzers
        std::vector<std::vector<std::shared_ptr<BaseAnalyzer>>> analyzers;
        
        //Vector of trees for each analysis
        std::vector<TTree*> trees;

        //Progress bar function
        void ProgressBar(const int &progress);
        

    public:
        NanoSkimmer();
        NanoSkimmer(const std::string &inFile, const bool &isData);
        void Configure(const float &xSec = 1.);
        void EventLoop(const std::vector<std::string> &channels);
        void WriteOutput(const std::string &outFile); 
};
