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
#include <fstream>

#include <TROOT.h>
#include <TFile.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TTree.h>

#include <TMath.h>
#include <Math/GenVector/LorentzVector.h>
#include <Math/GenVector/VectorUtil.h>
#include <Math/Vector3Dfwd.h>
#include <Math/Vector4Dfwd.h>

#include <ChargedAnalysis/Network/include/bdt.h>
#include <ChargedAnalysis/Network/include/htagger.h>
#include <ChargedAnalysis/Analysis/include/utils.h>

#include <torch/torch.h>

class TreeReader {
    private:
        //Enumeration for particles
        enum Particle{ELECTRON, MUON, JET, SUBJET, BSUBJET, BJET, FATJET, BFATJET, MET, W, HC, h, H1JET, H2JET, GENHC, GENH};

        //Enumeration for functions to calculate quantities
        enum Function{MASS, PHI, PT, ETA, DPHI, DR, NPART, HT, EVENTNUMBER, BDTSCORE, CONSTNUM, NSIGPART, SUBTINESS, HTAG};

        //Enumeration for cut operation
        enum Operator{EQUAL, BIGGER, SMALLER, EQBIGGER, EQSMALLER, DIVISIBLE, NOTDIVISIBLE};

        //Enumeration for safe mode
        enum SaveMode {HIST, TREE, CSV, TENSOR};

        //Struct for saving Particle information
        struct RecoParticle {
            ROOT::Math::PxPyPzEVector LV;
            int charge;

            //Booleans
            bool looseIso = true;
            bool mediumIso = true;
            bool tightIso = true;

            bool isLoose = true;
            bool isMedium = true;
            bool isTight = true;
            bool isTriggerMatched = true;

            int isFromSignal = -1.;

            //Scale factors
            std::vector<float> IDSF = {1., 1., 1.};
            float otherSF=1.;

            //Subtiness for fat jets
            std::vector<float> subtiness = {1., 1., 1.};

            //Define add operator
            RecoParticle operator+(const RecoParticle& other){
                RecoParticle newPart;
                newPart.LV = this->LV + other.LV;
                return newPart;
            }
        };

        //Struct for reading out tree
        struct Event{ 
            //Map with particles      
            std::map<Particle, std::vector<RecoParticle>> particles;

            int loopNr = -1;
            float HT=1.;
            float eventNumber;

            //Event weight
            float weight=1.;

            //Clear Function
            void Clear(){
                //Initialize particle map with empty vectors
                for(int i=0; i!=GENH; ++i){
                    particles[(Particle)i] = {};
                }

                loopNr++;
                weight=1.;
            }
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

        //Needed parameter set in the constructor
        std::string process;
        std::vector<std::string> xParameters; 
        std::vector<std::string> yParameters;
        std::vector<std::string> cutStrings;
        std::string outname;
        std::string channel;

        //Vector with cut information
        std::vector<Hist> cuts;
        std::vector<std::string> cutNames;
        bool writeCutFlow = false;

        //Variable to check what to save
        SaveMode toSave;
    
        //Methods to calculate wished quantity (Defined in treereaderfunction.cc)
        float Mass(Event &event, Hist &hist);
        float Phi(Event &event, Hist &hist);
        float Pt(Event &event, Hist &hist);
        float Eta(Event &event, Hist &hist);

        float DeltaPhi(Event &event, Hist &hist);
        float DeltaR(Event &event, Hist &hist);

        float Subtiness(Event &event, Hist &hist);
        float HadronicEnergy(Event &event, Hist &hist);
        float EventNumber(Event &event, Hist &hist);
        float ConstantNumber(Event &event, Hist &hist);

        float BDTScore(Event &event, Hist &hist);
        bool isBDT = false;
        //Function to get values for input parameter for BDT evaluation
        std::vector<Hist> bdtFunctions;
        BDT evenClassifier;
        BDT oddClassifier;

        void JetParameter(const std::string& fileName, const int& start, const int& end, std::vector<std::vector<float>>* jetParam=NULL);
        float HTag(Event &event, Hist &hist);
        bool isHTag = false;
        std::vector<float> tagValues;

        float NSigParticle(Event &event, Hist &hist);
        float NParticle(Event &event, Hist &hist);

        //Mapping for IDS/sf
        std::function<bool(int, RecoParticle)> ID;
        std::function<float(int, RecoParticle)> SF;

        //Function for reconstruct objects
        void WBoson(Event &event);
        void Higgs(Event &event);
    
        //Function for cuts
        bool Cut(Event &event, Hist &hist);

        //Function for converting string into wished enumerations
        Hist ConvertStringToEnums(const std::string &input, const bool &isCutString = false);
        std::tuple<std::vector<Hist>, std::vector<std::vector<Hist>>> SetHistograms(TFile* outputFile);

    public:
        TreeReader();
        TreeReader(const std::string &process, const std::vector<std::string> &xParameters, const std::vector<std::string> &yParameters, const std::vector<std::string> &cutStrings, const std::string &outname, const std::string &channel, const std::string &saveMode = "Hist");

        void EventLoop(const std::string &fileName, const int &entryStart, const int &entryEnd, std::vector<std::vector<float>>* jetParam = NULL);
};

#endif
