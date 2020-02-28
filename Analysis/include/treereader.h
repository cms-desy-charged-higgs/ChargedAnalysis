#ifndef TREEREADER_H
#define TREEREADER_H

#include <iostream>
#include <vector>
#include <string>
#include <map>

#include <TROOT.h>
#include <TFile.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TTree.h>

#include <ChargedAnalysis/Network/include/bdt.h>
#include <ChargedAnalysis/Network/include/htagger.h>
#include <ChargedAnalysis/Utility/include/utils.h>

#include <ChargedAnalysis/Analysis/include/treefunction.h>

class TreeReader {
    private:
        std::vector<std::string> parameters;
        std::vector<std::string> cutStrings;
        std::string outname;
        std::string channel;

        void PrepareLoop(TFile* outFile);

        //std::vector<Function> functions;
        std::vector<TH1F*> hists;
        std::vector<FuncArgs> histArgs;
        std::vector<Function> histFunctions;

        std::vector<FuncArgs> cutArgs;
        std::vector<Function> cutFunctions;

    public:
        TreeReader();
        TreeReader(const std::vector<std::string> &parameters, const std::vector<std::string> &cutStrings, const std::string &outname, const std::string &channel);

        void EventLoop(const std::string &fileName, const int &entryStart, const int &entryEnd, const std::string& cleanJet);
};

#endif
