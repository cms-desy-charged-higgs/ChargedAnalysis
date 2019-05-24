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

#include <ChargedHiggs/analysis/interface/bdt.h>


class TreeReader {
    private:
        //Enumeration for particles
        enum Particle{ELECTRON, MUON, JET, SUBJET, BJET, FATJET, BFATJET, MET, W, HC, h};

        //Enumeration for functions to calculate quantities
        enum Function{MASS, PHI, PT, ETA, DPHI, DR, NPART, HT, EVENTNUMBER, BDTSCORE, CONSTNUM, NSIGPART};

        //Enumeration for cut operation
        enum Operator{EQUAL, BIGGER, SMALLER, EQBIGGER, EQSMALLER, DIVISIBLE, NOTDIVISIBLE};

        //Struct for reading out tree
        struct Event{ 
            //Initially given       
            std::vector<Electron> electrons;
            std::vector<Muon> muons;
            std::vector<Jet> jets;
            std::vector<Jet> subjets;
            std::vector<FatJet> fatjets;
            TLorentzVector MET;  
            float weight; 
            float HT;
            float eventNumber;

            //Reconstructed during processing of the event
            std::vector<TLorentzVector> h;
            TLorentzVector W;
            TLorentzVector Hc;

            //Dummy variable
            TLorentzVector dummy;
        };

        //Struct for saving Particle/Function and wished histogram
        struct Hist{
            TH1* hist1D;
            TH2* hist2D;
            std::vector<Particle> parts;
            std::vector<int> indeces;
            Function func;
            float funcValue;
            std::pair<Operator, float> cutValues;
        };

        //Maps for parameter strings/enumeration
        std::map<std::string, Particle> strToPart;
        std::map<Particle, std::string> partLabel;
        std::map<Particle, std::string> nPartLabel;
        
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
        std::vector<std::string> xParameters; 
        std::vector<std::string> yParameters;
        std::vector<std::string> cutStrings;
        std::string outname;
        std::string channel;

        //Class for locking thread unsafe operation
        std::mutex mutex;

        //Final histograms
        std::vector<Hist> merged1DHistograms;
        std::vector<std::vector<Hist>> merged2DHistograms;
        TFile* outputFile;

        //Vector with cut information
        std::vector<Hist> cuts;
        std::vector<std::string> cutNames;
        TH1F* cutflow;

        //Save tree if wished
        bool saveTree;
    
        //Progress bar function
        void ProgressBar(const int &progress);

        //Loop for each thread
        void ParallelisedLoop(const std::vector<TChain*> &v, const int &entryStart, const int &entryEnd, const float &nGen);

        //Function for converting string into wished enumerations
        Hist ConvertStringToEnums(std::string &input, const bool &isCutString = false);

        const TLorentzVector& GetParticle(Event &event, Particle &part, const int &index = 1);

        //Methods to calculate wished quantity (Defined in treereaderfunction.cc)
        float Mass(Event &event, Hist &hist);
        float Phi(Event &event, Hist &hist);
        float Pt(Event &event, Hist &hist);
        float Eta(Event &event, Hist &hist);

        float DeltaPhi(Event &event, Hist &hist);
        float DeltaR(Event &event, Hist &hist);

        float HadronicEnergy(Event &event, Hist &hist);
        float EventNumber(Event &event, Hist &hist);
        float ConstantNumber(Event &event, Hist &hist);

        float BDTScore(Event &event, Hist &hist);
        //Function to get values for input parameter for BDT evaluation
        std::map<std::thread::id, std::vector<Hist>> bdtFunctions;
        std::map<std::thread::id, BDT> evenClassifier;
        std::map<std::thread::id, BDT> oddClassifier;

        bool isBDT = false;

        float NSigParticle(Event &event, Hist &hist);
        float NParticle(Event &event, Hist &hist);
        //Mapping for IDS/sf
        std::map<float, std::pair<Bool_t Electron::*, float>> eleID;
        std::map<float, Float_t Electron::*> eleSF;

        std::map<float, std::pair<Bool_t Muon::*, Bool_t Muon::*>> muonID;
        std::map<float, std::pair<Float_t Muon::*, Float_t Muon::*>> muonSF;

        std::map<float, Bool_t Jet::*> bJetID;
        std::map<float, Float_t Jet::*> bJetSF;

        //Function for reconstruct objects
        void WBoson(Event &event);
        void Higgs(Event &event);
    
        //Function for cuts
        bool Cut(Event &event, Hist &hist);


    public:
        TreeReader();
        TreeReader(std::string &process, std::vector<std::string> &xParameters, std::vector<std::string> &yParameters, std::vector<std::string> &cutStrings, std::string &outname, std::string &channel, const bool& saveTree = false);
        void SetHistograms();
        void EventLoop(std::vector<std::string> &filenames, const float &frac = 1.0);
        void Write();
};

#endif
