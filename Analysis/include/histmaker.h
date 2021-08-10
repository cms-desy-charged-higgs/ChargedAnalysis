#ifndef HISTMAKER_H
#define HISTMAKER_H

#include <vector>
#include <string>
#include <memory>
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
        std::vector<std::string> regions, parameters, scaleSysts, parametersEst, cutStringsEst;
        std::map<std::string, std::string> outDir;
        std::string outFile, channel, fakeRateFile, promptRateFile;
        std::map<std::string, std::vector<std::string>> cutStrings, systDirs;
        int era;

        std::shared_ptr<TFile> inputFile;
        std::shared_ptr<TTree> inputTree;

        std::vector<std::shared_ptr<TH1F>> hists1D, hists1DSystUp, hists1DSystDown, 
                                           hists1DEst, hists1DSystUpEst, hists1DSystDownEst;
        std::vector<std::shared_ptr<TH2F>> hists2D, hists2DSystUp, hists2DSystDown,
                                           hists2DEst, hists2DSystUpEst, hists2DSystDownEst;

        std::vector<std::shared_ptr<TH1F>> eventCount, eventCountEst;
        std::vector<std::shared_ptr<TH1F>> eventCountSystUp, eventCountSystDown, eventCountSystUpEst, eventCountSystDownEst;

        std::vector<std::shared_ptr<TFile>> outFiles;

        std::vector<NTupleReader> hist1DFunctions, cutFunctions,leptonCutsTight, leptonCutsLoose;
        std::vector<std::pair<NTupleReader, NTupleReader>> hist2DFunctions;
        std::vector<Weighter> weight;

        void PrepareHists(const std::shared_ptr<TFile>& inFile, const std::shared_ptr<TTree> inTree, const std::experimental::source_location& location = std::experimental::source_location::current());

    public:
        HistMaker();
        HistMaker(const std::vector<std::string>& parameters, const std::vector<std::string>& regions, const std::map<std::string, std::vector<std::string>>& cutStrings, const std::map<std::string, std::string>& outDir, const std::string& outFile, const std::string &channel, const std::map<std::string, std::vector<std::string>>& systDirs, const std::vector<std::string>& scaleSysts, const std::vector<std::string>& parametersEst, const std::vector<std::string>& cutStringsEst, const std::string& fakeRateFile, const std::string& promptRateFile, const int& era = 2017);

        void Produce(const std::string& fileName, const int& eventStart, const int& eventEnd, const std::string& bkgYieldFac = "", const std::string& bkgType = "", const std::vector<std::string>& bkgYieldFacSyst = {});
};

#endif
