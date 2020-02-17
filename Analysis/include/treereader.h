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
        std::string process;
        std::vector<std::string> xParameters; 
        std::vector<std::string> yParameters;
        std::vector<std::string> cutStrings;
        std::string outname;
        std::string channel;

        void PrepareLoop(TFile* outFile);

        //std::vector<Function> functions;
        std::vector<TH1F*> hists;
        std::vector<FuncArgs> args;
        std::vector<Function> functions;

    public:
        TreeReader();
        TreeReader(const std::string &process, const std::vector<std::string> &xParameters, const std::vector<std::string> &yParameters, const std::vector<std::string> &cutStrings, const std::string &outname, const std::string &channel, const std::string &saveMode = "Hist");

        void EventLoop(const std::string &fileName, const int &entryStart, const int &entryEnd);
};

#endif
