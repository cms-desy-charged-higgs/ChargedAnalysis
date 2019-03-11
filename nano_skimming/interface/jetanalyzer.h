#ifndef JETANALYZER_H
#define JETANALYZER_H

#include <ChargedHiggs/nano_skimming/interface/baseanalyzer.h>

#include <random>

#include <CondFormats/BTauObjects/interface/BTagCalibration.h>
#include <CondTools/BTau/interface/BTagCalibrationReader.h>
#include <JetMETCorrections/Modules/interface/JetResolution.h>

//Jet class to be safed in tree
struct Jet {
    TLorentzVector fourVec;
    Bool_t isLooseB;
    Bool_t isMediumB;
    Bool_t isTightB;
    Float_t loosebTagSF = 1.;
    Float_t mediumbTagSF = 1.;
    Float_t tightbTagSF = 1.;

    TLorentzVector genVec;
    Bool_t isFromh1 = false;
    Bool_t isFromh2 = false;
};



class JetAnalyzer: public BaseAnalyzer{
    private:
        //Bool for checking if data file
        bool isData;

        //Map for SF files
        std::map<int, std::string> bTagSF;
        std::map<int, std::string> JMESF;
        std::map<int, std::string> JMEPtReso;

        //Values for bTag cuts
        std::map<int, std::vector<float>> bTagCuts; 

        //Classes for reading btag SF
        BTagCalibration calib;
        BTagCalibrationReader looseReader;
        BTagCalibrationReader mediumReader;
        BTagCalibrationReader tightReader;

        //Classes for reading jet energy SF 
        JME::JetParameters jetParameter;
        JME::JetResolution resolution;
        JME::JetResolutionScaleFactor resolution_sf;

        //Input for selecting jets
        int era;
        float ptCut;
        float etaCut;
        unsigned int minNJet;

        //TTreeReader Values
        std::unique_ptr<TTreeReaderArray<float>> jetPt;
        std::unique_ptr<TTreeReaderArray<float>> jetEta;
        std::unique_ptr<TTreeReaderArray<float>> jetPhi;
        std::unique_ptr<TTreeReaderArray<float>> jetMass;
        std::unique_ptr<TTreeReaderArray<int>> jetGenIdx;
        std::unique_ptr<TTreeReaderValue<float>> jetRho;
        std::unique_ptr<TTreeReaderArray<float>> jetDeepBValue;
        std::unique_ptr<TTreeReaderValue<float>> valueHT;

        std::unique_ptr<TTreeReaderArray<float>> genJetPt;
        std::unique_ptr<TTreeReaderArray<float>> genJetEta;
        std::unique_ptr<TTreeReaderArray<float>> genJetPhi;

        std::unique_ptr<TTreeReaderValue<float>> metPhi;
        std::unique_ptr<TTreeReaderValue<float>> metPt;

        //Parameter for HT
        float HT;

        //Valid jet collection
        std::vector<Jet> validJets;

        //met Lorentzvector
        TLorentzVector met;

        //Get JER smear factor
        float SmearEnergy(float &jetPt, float &jetPhi, float &jetEta, float &rho);

        //Set Gen particle information
        void SetGenParticles(Jet &validJet, const int &i);

    public:
        JetAnalyzer(const int &era, const float &ptCut, const float &etaCut, const int &minNJet);
        void BeginJob(TTreeReader &reader, TTree* tree, bool &isData);
        bool Analyze();
        void EndJob(TFile* file);
};

#endif
