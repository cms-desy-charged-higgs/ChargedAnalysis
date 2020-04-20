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
#include <ChargedAnalysis/Analysis/include/treefunction.h>

class TreeReader {
    private:
        std::vector<std::string> parameters;
        std::vector<std::string> cutStrings;
        std::string outname;
        std::string channel;

        std::vector<TH1F*> hists;
        std::vector<FuncArgs> histArgs;
        std::vector<Function> histFunctions;

        TTree* outTree = NULL;
        std::vector<FuncArgs> treeArgs;
        std::vector<Function> treeFunctions;
        std::vector<std::string> branchNames;
        std::vector<float> treeValues;

        Frame* frame = NULL;
        std::vector<FuncArgs> CSVArgs;
        std::vector<Function> CSVFunctions;
        std::vector<std::string> CSVNames;

        std::vector<FuncArgs> cutArgs;
        std::vector<Function> cutFunctions;
        std::vector<std::string> cutLabels;

        //Objects to hold values 
        std::map<Particle, std::vector<float>*> Px, Py, Pz, E, Isolation, looseSF, mediumSF, tightSF, triggerSF, recoSF, loosebTagSF, mediumbTagSF, tightbTagSF, FatJetIdx, isFromh, oneSubJettiness, twoSubJettiness, threeSubJettiness, looseIsoLooseSF, looseIsoMediumSF, looseIsoTightSF, tightIsoMediumSF, tightIsoTightSF, BScore, TrueFlavour;
        std::map<Particle, std::vector<bool>*> isLoose, isMedium, isTight, isLooseIso, isMediumIso, isTightIso, isLooseB, isMediumB, isTightB;

        std::vector<float> weights;
        float MET_Px, MET_Py, eventNumber, nTrue, nGen=1.;
        std::map<int, float> bdtScore, dnnScore;

        void PrepareLoop(TFile* outFile);

    public:
        TreeReader();
        TreeReader(const std::vector<std::string> &parameters, const std::vector<std::string> &cutStrings, const std::string &outname, const std::string &channel);

        void GetFunction(const std::string& parameter, Function& func, FuncArgs& args);
        void GetParticle(const std::string& parameter, FuncArgs& args);
        void GetCut(const std::string& parameter, FuncArgs& args);
        void GetBinning(const std::string& parameter, TH1* hist);

        template <typename TreeObject>
        void PrepareEvent(TreeObject inputTree);
        void SetEvent(Event& event, const Particle& cleanPart=JET, const WP& cleanWP=NONE);
        void EventLoop(const std::string &fileName, const int &entryStart, const int &entryEnd, const std::string& cleanJet);
};

#endif
