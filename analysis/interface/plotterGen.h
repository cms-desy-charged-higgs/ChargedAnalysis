#ifndef PLOTTERGEN_H
#define PLOTTERGEN_H

#include <ChargedHiggs/analysis/interface/plotter.h>

#include <functional>

#include <TFile.h>
#include <TTree.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>
#include <TTreeReaderArray.h>
#include <TH1F.h>
#include <TCanvas.h>
#include <TMath.h>
#include <TLorentzVector.h>

class PlotterGen : public Plotter{
    //Struct for Gen Level particles    
    struct genEvent{
        TLorentzVector Hc;
        TLorentzVector h1; //small higgs originating from Higgs strahlung
        TLorentzVector h2; 
        TLorentzVector W; //W originating from charged Higgs 
        TLorentzVector l;
        TLorentzVector vl;
        TLorentzVector b1; //From h1
        TLorentzVector b2;
        TLorentzVector b3; //From h2
        TLorentzVector b4;
    };

    struct histConfig{
        std::string name;
        float nBins;
        float Min;
        float Max;
        std::string Label;
        std::function<float(genEvent)> parameterValue;
    };

    private:
        std::string filename;
        std::vector<TH1F*> hists;
        std::vector<std::function<float(genEvent)>> valueFunctions;

    public:
        PlotterGen();
        PlotterGen(std::string &filename);
        void ConfigureHists(std::vector<std::string> &processes);
        void FillHists();
        void Draw(std::vector<std::string> &outdirs);
        
};

#endif
