#ifndef TREEREADER_H
#define TREEREADER_H

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <fstream>

#include <TROOT.h>
#include <TFile.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TTree.h>
#include <TChain.h>

#include <ChargedAnalysis/Utility/include/utils.h>
#include <ChargedAnalysis/Utility/include/frame.h>
#include <ChargedAnalysis/Analysis/include/treeparser.h>
#include <ChargedAnalysis/Analysis/include/treefunction.h>

class TreeReader {
    private:
        std::vector<std::string> parameters;
        std::vector<std::string> cutStrings;
        std::string outname;
        std::string channel;

        std::vector<TH1F*> hists1D;
        std::vector<TreeFunction> hist1DFunctions;

        std::vector<TH2F*> hists2D;
        std::vector<TreeFunction> hist2DFunctions;

        TTree* outTree = nullptr;
        std::vector<TreeFunction> treeFunctions;
        std::vector<std::string> branchNames;
        std::vector<float> treeValues;

        Frame* frame = nullptr;
        std::vector<TreeFunction> CSVFunctions;
        std::vector<std::string> CSVNames;

        std::vector<TreeFunction> cutFunctions;

        int nGen = 1, nTrue = 0;
        float lumi = 1., xSec = 1.;

        void PrepareLoop(TFile* outFile, TTree* inputTree);

    public:
        TreeReader();
        TreeReader(const std::vector<std::string> &parameters, const std::vector<std::string> &cutStrings, const std::string &outname, const std::string &channel);

        void EventLoop(const std::string &fileName, const int &entryStart, const int &entryEnd, const std::string& cleanJet);
};

#endif
