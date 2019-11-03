#ifndef PLOTTERCUT_H
#define PLOTTERCUT_H

#include <ChargedAnalysis/Analysis/include/plotter.h>

#include <TFile.h>
#include <TH1F.h>
#include <TLegend.h>
#include <TCanvas.h>
#include <TPad.h>
#include <Rtypes.h>

#include <sstream>
#include <iostream>

class PlotterCut : public Plotter{
    
    private:
        std::vector<std::vector<TH1F*>> cutHists; 
        std::vector<std::vector<TH1F*>> NsigHists; 
        std::vector<TH1F*> NbkgHists;

        std::string channel;
        std::vector<std::string> xParameters;
        std::vector<std::string> processes;

        Double_t GetAsimov(const Double_t &s, const Double_t &b, const Double_t &sigB, const Double_t &sigS);

    public:
        PlotterCut();
        PlotterCut(std::string &histdir, std::vector<std::string> &xParameters, std::string &channel, std::vector<std::string> &processes);
        void ConfigureHists();
        void Draw(std::vector<std::string> &outdirs);
};

#endif
