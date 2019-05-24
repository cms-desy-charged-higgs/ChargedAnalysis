#ifndef BASEANALYZER_H
#define BASEANALYZER_H

#include <memory>
#include <map>
#include <vector>
#include <cmath>
#include <bitset>

#include <TFile.h>
#include <TH2F.h>
#include <Rtypes.h>
#include <TLorentzVector.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>
#include <TTreeReaderArray.h>

//Struct for saving gen particle lorentz vectors
struct GenParticles{
    TLorentzVector W;
    TLorentzVector h1;
    TLorentzVector h2;
    TLorentzVector Hc;
};

class BaseAnalyzer{
    protected:
        //File path for SF etc.
        std::string filePath = std::string(std::getenv("CMSSW_BASE")) + "/src/ChargedHiggs/nano_skimming/data/";

        //Collection which are used in several analyzers
        std::unique_ptr<TTreeReaderArray<float>> trigObjPhi;
        std::unique_ptr<TTreeReaderArray<float>> trigObjEta;
        std::unique_ptr<TTreeReaderArray<int>> trigObjID;
        std::unique_ptr<TTreeReaderArray<int>> trigObjFilterBit;
    
        std::unique_ptr<TTreeReaderArray<float>> genPhi;
        std::unique_ptr<TTreeReaderArray<float>> genEta;
        std::unique_ptr<TTreeReaderArray<float>> genPt;
        std::unique_ptr<TTreeReaderArray<float>> genMass;
        std::unique_ptr<TTreeReaderArray<int>> genID;
        std::unique_ptr<TTreeReaderArray<int>> genMotherIdx;
        std::unique_ptr<TTreeReaderArray<int>> genStatus;

        //Set trihObj and Gen particle collection
        void SetCollection(TTreeReader &reader, bool &isData);
        
        //Trigger matching
        bool triggerMatching(const TLorentzVector &particle, const int &particleID);

    public:
        virtual ~BaseAnalyzer(){};
        BaseAnalyzer();
        virtual void BeginJob(TTreeReader &reader, TTree *tree, bool &isData) = 0;
        virtual bool Analyze(std::pair<TH1F*, float> &cutflow) = 0;
        virtual void EndJob(TFile* file) = 0;
};

#endif
