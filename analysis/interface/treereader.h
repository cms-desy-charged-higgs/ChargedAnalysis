#ifndef TREEREADER_H
#define TREEREADER_H

#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <map>

#include <TLorentzVector.h>
#include <TFile.h>
#include <TH1F.h>
#include <TTree.h>
#include <TChain.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>

#include <ChargedHiggs/nanoAOD_processing/macros/quantities.cc> 
#include <ChargedHiggs/nanoAOD_processing/macros/jet.cc> 
#include <ChargedHiggs/nanoAOD_processing/macros/lepton.cc> 


class TreeReader{
    
    enum Processes {DY, QCD, TT, T, DATA, VV, WJ};

    //Struct for reading out tree
    struct Event{
        std::vector<Lepton> leptons;
        std::vector<Jet> jets;
        Quantities quantities;
        TLorentzVector MET; 
    };

    //Struct for histogram configuration
    struct Hist{
        float nBins;
        float xMin;
        float xMax;
        std::function<float (Event)> histFunc;
    };

    private:
        std::string process;
    
        std::map<std::string, Hist> histValues; 
        std::map<std::string, std::function<bool (Event)>> cutValues; 
        std::map<std::string, Processes> procDic;               
        std::map<TH1F*, std::function<float (Event)>> histograms;     
        
        void SetHistMap();
        void SetCutMap();
    
        float GetWeight(Event event, std::vector<float> weights);

        //Functions for cuts 

        static std::function<bool (Event)> Baseline;

        //Functions for calculating quantities
        static std::function<float (Event)> WBosonMT;
        static std::function<float (Event)> WBosonPhi;
        static std::function<float (Event)> WBosonPT;
        static std::function<float (Event)> ElectronPT;
        static std::function<float (Event)> ElectronPhi;
        static std::function<float (Event)> ElectronEta;
        static std::function<float (Event)> Jet1PT;
        static std::function<float (Event)> Jet1Phi;
        static std::function<float (Event)> Jet1Eta;
        static std::function<float (Event)> Jet2PT;
        static std::function<float (Event)> Jet2Phi;
        static std::function<float (Event)> Jet2Eta;
        static std::function<float (Event)> nJet;
        static std::function<float (Event)> nLooseBJet;
        static std::function<float (Event)> nMediumBJet;
        static std::function<float (Event)> nTightBJet;
        static std::function<float (Event)> HT;
        static std::function<float (Event)> MET;

    public:
        TreeReader();
        TreeReader(std::string &process);
        void AddHistogram(std::vector<std::string> &parameters);
        void EventLoop(std::vector<std::string> &filenames, std::vector<std::string> &cutstrings);
        void Write(std::string &outname);
};


#endif
