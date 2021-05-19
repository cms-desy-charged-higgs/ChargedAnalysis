#ifndef WEIGHTER_H
#define WEIGHTER_H

#include <vector>
#include <string>
#include <memory>
#include <functional>

#include <TFile.h>
#include <TTree.h>
#include <TH1F.h>
#include <TLeaf.h>
#include <TH2F.h>
#include <TParameter.h>

#include <ChargedAnalysis/Analysis/include/ntuplereader.h>
#include <ChargedAnalysis/Utility/include/rootutil.h>
#include <ChargedAnalysis/Utility/include/stringutil.h>

namespace pt = boost::property_tree;

class NTupleReader;

class Weighter{
    private:
        std::shared_ptr<TFile> inputFile;
        std::shared_ptr<TTree> inputTree;
        int era;
        bool isData;
        double xSec = 1., lumi = 1.;
        std::vector<double> stitchedWeights;
        TLeaf* nPartons = nullptr;

        double nGen = 1., baseWeight = 1., partWeight = 1.;
        std::shared_ptr<TH1F> pileUpWeight, pileUpWeightUp, pileUpWeightDown;
        std::vector<NTupleReader> sf, sfUp, sfDown;
        std::function<float(const int&)> bWeight, bWeightUp, bWeightDown; 
        std::vector<std::string> systematics;

        static double GetBJetWeight(const int& entry, TH2F* effB, TH2F* effC, TH2F* effLight, NTupleReader bPt, TLeaf* pt, TLeaf* eta, TLeaf* sf, TLeaf* flavour);

    public:
        Weighter();
        Weighter(const std::shared_ptr<TFile>& inputFile, const std::shared_ptr<TTree>& inputTree, const int& era = 2017);

        void AddParticle(const std::string& partName, const std::string& wp, const std::experimental::source_location& location = std::experimental::source_location::current());

        int GetNWeights();

        double GetNGen();
        double GetBaseWeight(const std::size_t& entry, const std::string& sysShift = "");
        double GetPartWeight(const std::size_t& entry, const std::string& syst = "", const std::string& sysShift = "");
        double GetPartWeightByIdx(const std::size_t& entry, const int& idx, const std::string& sysShift = "");
        double GetTotalWeight(const std::size_t& entry, const std::string& syst = "", const std::string& sysShift = "");
};

#endif
