#ifndef TREEREADER_H
#define TREEREADER_H

#include <vector>
#include <string>
#include <functional>
#include <map>
#include <thread>
#include <mutex>

#include <TLorentzVector.h>
#include <TFile.h>
#include <TH1F.h>
#include <TChain.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>

#include <ChargedHiggs/nanoAOD_processing/macros/jet.cc> 
#include <ChargedHiggs/nanoAOD_processing/macros/lepton.cc> 

class TreeReader{
    
    enum Processes {DY, QCD, TT, T, DATA, VV, WJ};

    //Struct for reading out tree
    struct Event{
        std::vector<Lepton> leptons;
        std::vector<Jet> jets;
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
        std::map<std::string, Processes> procDic;               
        std::map<TH1F*, std::function<float (Event)>> histograms;     
        
        void SetHistMap();
        float GetWeight(Event event, std::vector<float> weights);
        bool Cut(Event event);

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
        static std::function<float (Event)> nJet;

    public:
        TreeReader();
        TreeReader(std::string &process);
        void AddHistogram(std::vector<std::string> &parameters);
        void EventLoop(std::vector<std::string> &filenames);
        void Write(std::string &outname);
};


#endif
