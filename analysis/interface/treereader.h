#ifndef TREEREADER_H
#define TREEREADER_H

#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <map> 
#include <cmath>
#include <set>	
#include <tuple>

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
    private:
        //Enumeration for particles
        enum Particle{ELECTRON, MUON, JET, BJET, FATJET, BFATJET, MET, W, HC, h};

        //Enumeration for functions to calculate quantities
        enum Function{MASS, PHI, PT, ETA, DPHI, DR, LOOSENPART, MEDIUMNPART, TIGHTNPART, HT};

        //Enumeration for cut operation
        enum Operator{EQUAL, BIGGER, SMALLER, EQBIGGER, EQSMALLER};

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

            //Reconstructed during processing of the event
            std::vector<TLorentzVector> h;
            TLorentzVector W;
            TLorentzVector Hc;

            //Dummy variable
            TLorentzVector dummy;
        };

        //Struct for saving Particle/Function and wished histogram
        struct Hist{
            TH1F* hist;
            std::vector<Particle> parts;
            std::vector<int> indeces;
            Function func;
            std::pair<Operator, float> cutValues;
        };

        //Maps for parameter strings/enumeration
        std::map<std::string, Particle> strToPart;
        std::map<Particle, std::string> partLabel;
        
        //Maps for function strings/enumeration
        std::map<std::string, Function> strToFunc;
        std::map<Function, std::string> funcLabel;
        std::map<Function, std::vector<float>> binning;
        std::map<Function, float(TreeReader::*)(Event &event, Hist &hist)> funcDir;

        //Map for cut string/enumeration 
        std::map<std::string, Operator> strToOp;

        int progress = 0;
        int nCores;
        unsigned int nHist = 0.;

        //Measure execution time
        std::chrono::steady_clock::time_point start;
        std::chrono::steady_clock::time_point end;

        //Needed parameter set in the constructor
        std::string process;

        //Class for locking thread unsafe operation
        std::mutex mutex;

        //Final histograms
        std::vector<Hist> merged1DHistograms;

        //Vector with cut information
        std::vector<Hist> cuts;

        //Save tree if wished
        bool saveTree;
        TList* listTree;
        TTree* outTree;
    
        //Progress bar function
        void ProgressBar(const int &progress);

        //Loop for each thread
        void ParallelisedLoop(const std::vector<TChain*> &v, const int &entryStart, const int &entryEnd, const float& nGen);

        const TLorentzVector& GetParticle(Event &event, Particle &part, const int &index = 1);

        //Methods to calculate wished quantity
        float Mass(Event &event, Hist &hist);
        float Phi(Event &event, Hist &hist);
        float Pt(Event &event, Hist &hist);
        float Eta(Event &event, Hist &hist);

        float DeltaPhi(Event &event, Hist &hist);
        float DeltaR(Event &event, Hist &hist);

        float HadronicEnergy(Event &event, Hist &hist);

        float NParticle(Event &event, Hist &hist);
        //Mapping for IDS/sf
        std::map<Function, std::pair<Bool_t Electron::*, float>> eleID;
        std::map<Function, Float_t Electron::*> eleSF;

        std::map<Function, std::pair<Bool_t Muon::*, Bool_t Muon::*>> muonID;
        std::map<Function, std::pair<Float_t Muon::*, Float_t Muon::*>> muonSF;

        std::map<Function, Bool_t Jet::*> bJetID;
        std::map<Function, Float_t Jet::*> bJetSF;

        //Function for reconstruct objects
        void WBoson(Event &event);
        void Higgs(Event &event);
    
        //Function for cuts
        bool Cut(Event &event, Hist &hist);


    public:
        TreeReader();
        TreeReader(std::string &process, const bool &saveTree = false);
        void SetHistograms(std::vector<std::string> &xParameters, std::vector<std::string> &yParameters, std::vector<std::string> &cuts);
        void EventLoop(std::vector<std::string> &filenames, std::string &channel);
        void Write(std::string &outname);
};

#endif
