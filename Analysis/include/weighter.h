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

#include <ChargedAnalysis/Analysis/include/ntuplereader.h>
#include <ChargedAnalysis/Utility/include/rootutil.h>
#include <ChargedAnalysis/Utility/include/stringutil.h>

namespace pt = boost::property_tree;

class Weighter{
    private:
        std::shared_ptr<TFile> inputFile;
        std::shared_ptr<TTree> inputTree;
        int era;
        bool isData;

        double baseWeight = 1., partWeight = 1.;
        std::shared_ptr<TH1F> pileUpWeight, pileUpWeightUp, pileUpWeightDown;
        std::vector<NTupleReader> sf, sfUp, sfDown;
        std::function<float(const int&)> bWeight, bWeightUp, bWeightDown; 

        static double GetBJetWeight(const int& entry, TH2F* effB, TH2F* effC, TH2F* effLight, NTupleReader bPt, TLeaf* pt, TLeaf* eta, TLeaf* sf, TLeaf* flavour);

    public:
        Weighter();
        Weighter(const std::shared_ptr<TFile>& inputFile, const std::shared_ptr<TTree>& inputTree, const int& era = 2017);

        void AddParticle(const std::string& partName, const std::string& wp, const std::experimental::source_location& location = std::experimental::source_location::current());

        double GetBaseWeight(const std::size_t& entry);
        double GetPartWeight(const std::size_t& entry);
        double GetTotalWeight(const std::size_t& entry);
};

#endif
