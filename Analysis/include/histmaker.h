#ifndef HISTMAKER_H
#define HISTMAKER_H

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <experimental/source_location>

#include <TROOT.h>
#include <TFile.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TTree.h>

#include <ChargedAnalysis/Analysis/include/ntuplereader.h>
#include <ChargedAnalysis/Analysis/include/weighter.h>
#include <ChargedAnalysis/Analysis/include/decoder.h>
#include <ChargedAnalysis/Utility/include/csv.h>
#include <ChargedAnalysis/Utility/include/stopwatch.h>
#include <ChargedAnalysis/Utility/include/stringutil.h>
#include <ChargedAnalysis/Utility/include/rootutil.h>
#include <ChargedAnalysis/Utility/include/vectorutil.h>

class HistMaker {
    private:
        std::vector<std::string> regions, parameters, scaleSysts;
        std::map<std::string, std::string> outDir;
        std::string outFile, channel, fakeRateFile, promptRateFile;
        std::map<std::string, std::vector<std::string>> cutStrings, systDirs;
        int era;
        bool isMisIDJ;

        std::shared_ptr<TFile> inputFile;
        std::shared_ptr<TTree> inputTree;

        std::vector<std::shared_ptr<TFile>> outFiles, outFilesForMisIDJ;
        std::vector<std::shared_ptr<TH1F>> hists1D, hists1DMisIDJ, hists1DSystUp, hists1DSystDown;
        std::vector<std::shared_ptr<TH2F>> hists2D, hists2DMisIDJ, hists2DSystUp, hists2DSystDown;
        std::vector<std::shared_ptr<TH1F>> eventCount, eventCountMisIDJ, eventCountSystUp, eventCountSystDown;

        std::vector<int> cutIdxRange;
        std::vector<NTupleFunction> hist1DFunctions, cutFunctions;
        std::vector<std::pair<NTupleFunction, NTupleFunction>> hist2DFunctions;

        Weighter baseWeight;
        std::unordered_map<int, Weighter> cutPartWeight, histPartWeight;

        void PrepareHists(const std::shared_ptr<TFile>& inFile, const std::shared_ptr<TTree> inTree, NTupleReader& reader, const std::experimental::source_location& location = std::experimental::source_location::current());

    public:
        HistMaker();
        HistMaker(const std::vector<std::string>& parameters, const std::vector<std::string>& regions, const std::map<std::string, std::vector<std::string>>& cutStrings, const std::map<std::string, std::string>& outDir, const std::string& outFile, const std::string &channel, const std::map<std::string, std::vector<std::string>>& systDirs, const std::vector<std::string>& scaleSysts, const std::string& fakeRateFile, const std::string& promptRateFile, const int& era = 2017);

        void Produce(const std::string& fileName, const int& eventStart, const int& eventEnd, const std::string& bkgYieldFac = "", const std::string& bkgType = "", const std::vector<std::string>& bkgYieldFacSyst = {});
};

#endif
