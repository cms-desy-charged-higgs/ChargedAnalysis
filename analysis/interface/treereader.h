#ifndef TREEREADER_H
#define TREEREADER_H

#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <map> 
#include <cmath>	

#include <thread>
#include <mutex>
#include <chrono>
#include <pthread.h>

#include <TROOT.h>
#include <TLorentzVector.h>
#include <TFile.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TTree.h>
#include <TChain.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>
#include <TMath.h>

#include <ChargedHiggs/nano_skimming/interface/electronanalyzer.h>
#include <ChargedHiggs/nano_skimming/interface/muonanalyzer.h> 
#include <ChargedHiggs/nano_skimming/interface/jetanalyzer.h>


class TreeReader {
    //Enumeration for trigger names
    enum Trigger{MET200, ELE35, ELE30JET35, ELE28HT150, MU27};

    //Struct for reading out tree
    struct Event{ 
        //Initially given       
        std::vector<Electron> electrons;
        std::vector<Muon> muons;
        std::vector<Jet> jets;
        std::vector<Jet> fatjets;
        TLorentzVector MET;  
        float weight; 
        float HT;

        std::map<Trigger, int> trigger;

        //Reconstructed during processing of the event
        TLorentzVector h1;
        TLorentzVector h2;
        TLorentzVector W;
        TLorentzVector Hc;

        TLorentzVector top1;
        TLorentzVector top2;
    };

    //typedef for pointer to member functions
    typedef float(TreeReader::*parameterFunc)(Event &event);
    typedef bool(TreeReader::*cut)(Event &event);

    //Struct for histogram configuration
    struct Hist{
        float nBins;
        float Min;
        float Max;
        std::string Label;
        parameterFunc parameterValue;
    };

    private:
        int progress = 0;
        int nCores;

        //Measure execution time
        std::chrono::steady_clock::time_point start;
        std::chrono::steady_clock::time_point end;

        //Needed parameter set in the constructor
        std::string process;
        std::vector<std::string> xParameters;
        std::vector<std::string> yParameters;
        std::vector<std::string> cutstrings;

        //Class for locking thread unsafe operation
        std::mutex mutex;

        //Final histograms
        std::vector<TH1F*> merged1DHistograms;
        std::vector<std::vector<TH2F*>> merged2DHistograms;

        //Save tree if wished
        bool saveTree;
        TList* listTree;
        TTree* outTree;

        //Helper function
        std::map<std::string, std::pair<Trigger, std::string>> trigNames;
        std::map<std::string, Hist> histValues; 
        std::map<std::string, cut> cutValues;                 

        //Setup fill in treereaderfunction.cc        
        void SetHistMap();
        void SetCutMap();
        void SetTriggerMap();
    
        //Progress bar function
        void ProgressBar(int progress);

        //Loop for each thread
        void ParallelisedLoop(const std::vector<TChain*> &v, const int &entryStart, const int &entryEnd, const float& nGen);

        //Function for reconstruct objects
        void Top(Event &event);
        void WBoson(Event &event);
        void Higgs(Event &event);

        //Functions for cuts 
        bool mediumSingleElectron(Event &event);
        bool mediumSingleMuon(Event &event);
        bool mediumDoubleElectron(Event &event);
        bool mediumDoubleMuon(Event &event);
        bool NonIsoElectron(Event &event);
        bool ZeroBJets(Event &event);
        bool OneBJets(Event &event);
        bool TwoBJets(Event &event);
        bool ThreeBJets(Event &event);
        bool XBJets(Event &event);
        bool TwoJetsOneFat(Event &event);
        bool FourJets(Event &event);
        bool MassCut(Event &event);
        bool AntiMassCut(Event &event);
        bool PhiCut(Event &event);

        //Functions for calculating quantities
        float HLTEle30Jet35(Event &event);
        float HLTEle28HT150(Event &event);
        float HLTEle35(Event &event);
        float HLTMu27(Event &event);
        float HLTMet200(Event &event);
        float HLTEle35AndMet200(Event &event);
        float HLTEle30Jet35AndMet200(Event &event);
        float HLTEle28HT150AndMet200(Event &event);
        float HLTEleAllAndMet200(Event &event);
        float HLTMu27AndMet200(Event &event);
        float top1Mass(Event &event);
        float top2Mass(Event &event);
        float top1PT(Event &event);
        float top2PT(Event &event);
        float dPhitop1top2(Event &event);
        float dRtop1top2(Event &event);
        float dMtop1top2(Event &event);
        float dPhitop1W(Event &event);
        float dPhitop2W(Event &event);
        float WBosonMT(Event &event);
        float WBosonPhi(Event &event);
        float WBosonPT(Event &event);
        float nElectron(Event &event);
        float nMuon(Event &event);
        float DiEleMass(Event &event);
        float ElectronPT(Event &event);
        float ElectronPhi(Event &event);
        float ElectronEta(Event &event);
        float MuonPT(Event &event);
        float MuonPhi(Event &event);
        float MuonEta(Event &event);
        float Jet1PT(Event &event);
        float Jet1Phi(Event &event);
        float Jet1Eta(Event &event);
        float Jet2PT(Event &event);
        float Jet2Phi(Event &event);
        float Jet2Eta(Event &event);
        float nJet(Event &event);
        float nFatJet(Event &event);
        float nLooseBJet(Event &event);
        float nMediumBJet(Event &event);
        float nTightBJet(Event &event);
        float HT(Event &event);
        float MET(Event &event);
        float METPhi(Event &event);
        float higgs1Mass(Event &event);
        float higgs2Mass(Event &event);
        float higgs1PT(Event &event);
        float higgs2PT(Event &event);
        float higgs1Phi(Event &event);
        float higgs2Phi(Event &event);
        float dMh1h2(Event &event);
        float cHiggsPT(Event &event);
        float cHiggsMT(Event &event);
        float cHiggsM(Event &event);
        float dPhih1h2(Event &event);
        float dRh1W(Event &event);
        float dRh2W(Event &event);
        float dRh1Hc(Event &event);
        float dRh2Hc(Event &event);
        float dPhih1W(Event &event);
        float dPhih2W(Event &event);
        float dPhih1Hc(Event &event);
        float dPhih2Hc(Event &event);
        float Mh1h2(Event &event);
        float SumLepJet(Event &event);
        float VecSumLepJet(Event &event);
        float dPth2Hc(Event &event);
        float RMHcMTop(Event &event);


    public:
        TreeReader();
        TreeReader(std::string &process, std::vector<std::string> &xParameters, std::vector<std::string> &yParameters, std::vector<std::string> &cutstrings, const bool &saveTree = false);
        void EventLoop(std::vector<std::string> &filenames, std::string &channel);
        void Write(std::string &outname);
};


#endif
