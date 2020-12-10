#ifndef TREEREADER_H
#define TREEREADER_H

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <memory>
#include <filesystem>

#include <TROOT.h>
#include <TFile.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TTree.h>
#include <TChain.h>

#include <ChargedAnalysis/Utility/include/utils.h>
#include <ChargedAnalysis/Utility/include/frame.h>
#include <ChargedAnalysis/Utility/include/csv.h>
#include <ChargedAnalysis/Utility/include/stringutil.h>
#include <ChargedAnalysis/Utility/include/rootutil.h>
#include <ChargedAnalysis/Utility/include/mathutil.h>
#include <ChargedAnalysis/Analysis/include/treeparser.h>
#include <ChargedAnalysis/Analysis/include/treefunction.h>

class TreeReader {
    private:
        std::vector<std::string> parameters;
        std::vector<std::string> cutStrings;
        std::string outDir;
        std::string outFile;
        std::string channel;
        std::vector<std::string> systDirs;
        std::vector<std::string> scaleSysts;
        std::string scaleFactors;
        int era;

        std::shared_ptr<TFile> inputFile;
        std::shared_ptr<TTree> inputTree;

        std::vector<std::shared_ptr<TH1F>> hists1D;
        std::vector<std::shared_ptr<TH2F>> hists2D;
        std::vector<std::shared_ptr<TH1F>> hists1DSyst;
        std::vector<std::shared_ptr<TH2F>> hists2DSyst;
        std::shared_ptr<TTree> outTree;
        std::shared_ptr<Frame> frame;

        std::vector<std::shared_ptr<TFile>> outFiles;

        std::vector<TreeFunction> hist1DFunctions;
        std::vector<TreeFunction> hist2DFunctions;
        std::vector<TreeFunction> cutFunctions;
        std::vector<TreeFunction> treeFunctions;
        std::vector<TreeFunction> CSVFunctions;

        std::vector<std::string> branchNames;
        std::vector<std::string> CSVNames;

        std::vector<float> treeValues;

        int nGen = 1, nTrue = 0;
        float lumi = 1., xSec = 1.;

        void PrepareLoop();

    public:
        TreeReader();
        TreeReader(const std::vector<std::string> &parameters, const std::vector<std::string> &cutStrings, const std::string& outDir, const std::string &outFile, const std::string &channel, const std::vector<std::string>& systDirs, const std::vector<std::string>& scaleSysts, const std::string& scaleFactors, const int& era = 2017);

        void EventLoop(const std::string& fileName, const std::string& cleanJet);
};

#endif
