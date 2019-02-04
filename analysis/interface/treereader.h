#ifndef TREEREADER_H
#define TREEREADER_H

#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <map> 	

#include <thread>
#include <mutex>
#include <chrono>
#include <pthread.h>

#include <TROOT.h>
#include <TLorentzVector.h>
#include <TFile.h>
#include <TH1F.h>
#include <TTree.h>
#include <TChain.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>

#include <ChargedHiggs/nanoAOD_processing/interface/quantities.h> 
#include <ChargedHiggs/nanoAOD_processing/interface/jet.h> 
#include <ChargedHiggs/nanoAOD_processing/interface/lepton.h> 


class TreeReader {
    
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
        int progress = 0;
        int nCores;

        //Needed parameter set in the constructor
        std::string process;
        std::vector<std::string> parameters;
        std::vector<std::string> cutstrings;

        //Class for locking thread unsafe operation
        std::mutex mutex;

        //Final histograms
        std::vector<TH1F*> mergedHistograms;

        //Helper function
        std::map<std::string, Hist> histValues; 
        std::map<std::string, std::function<bool (Event)>> cutValues; 
        std::map<std::string, Processes> procDic;                   

        //Setup fill in treereaderfunction.cc        
        void SetHistMap();
        void SetCutMap();
    
        //Helper function
        void ProgressBar(int progress);
        float GetWeight(Event event, std::vector<float> weights);

        //Loop for each thread
        void ParallelisedLoop(const std::vector<TChain*> &v, const int &entryStart, const int &entryEnd, const float nGen);

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
        static std::function<float (Event)> METPhi;

    public:
        TreeReader();
        TreeReader(std::string &process, std::vector<std::string> &parameters, std::vector<std::string> &cutstrings);
        void EventLoop(std::vector<std::string> &filenames);
        void Write(std::string &outname);
};


#endif
