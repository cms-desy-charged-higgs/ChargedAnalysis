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

        std::vector<TH1F*> hists;
        std::vector<FuncArgs> histArgs;
        std::vector<Function> histFunctions;

        TTree* outTree = NULL;
        std::vector<FuncArgs> treeArgs;
        std::vector<Function> treeFunctions;
        std::vector<std::string> branchNames;
        std::vector<float> treeValues;

        std::vector<FuncArgs> cutArgs;
        std::vector<Function> cutFunctions;
        std::vector<std::string> cutLabels;

    public:
        TreeReader();
        TreeReader(const std::vector<std::string> &parameters, const std::vector<std::string> &cutStrings, const std::string &outname, const std::string &channel);

        static void GetFunction(const std::string& parameter, Function& func);
        static void GetParticle(const std::string& parameter, FuncArgs& args);
        static void GetCut(const std::string& parameter, FuncArgs& args);
        static void GetBinning(const std::string& parameter, TH1* hist);

        void EventLoop(const std::string &fileName, const int &entryStart, const int &entryEnd, const std::string& cleanJet);
};

#endif
