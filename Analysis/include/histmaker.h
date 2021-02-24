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
        std::vector<std::string> parameters, cutStrings;
        std::string outDir, outFile, channel;
        std::vector<std::string> systDirs, scaleSysts;
        std::string scaleFactors;
        int era;

        std::shared_ptr<TFile> inputFile;
        std::shared_ptr<TTree> inputTree;

        std::vector<std::shared_ptr<TH1F>> hists1D, hists1DSyst;
        std::vector<std::shared_ptr<TH2F>> hists2D, hists2DSyst;

        std::vector<std::shared_ptr<TFile>> outFiles;

        std::vector<NTupleReader> hist1DFunctions, cutFunctions;
        std::vector<std::pair<NTupleReader, NTupleReader>> hist2DFunctions;
        Weighter weight;

        void PrepareHists(const std::experimental::source_location& location = std::experimental::source_location::current());

    public:
        HistMaker();
        HistMaker(const std::vector<std::string>& parameters, const std::vector<std::string>& cutStrings, const std::string& outDir, const std::string& outFile, const std::string& channel, const std::vector<std::string>& systDirs, const std::vector<std::string>& scaleSysts, const std::string& scaleFactors, const int& era = 2017);

        void Produce(const std::string& fileName);
};

#endif
