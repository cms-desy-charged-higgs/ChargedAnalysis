#ifndef PLOTTER1D_H
#define PLOTTER1D_H

#include <ChargedAnalysis/Analysis/include/plotter.h>

#include <TFile.h>
#include <TH1F.h>
#include <TGraph.h>
#include <THStack.h>
#include <TLegend.h>
#include <TCanvas.h>
#include <TPad.h>
#include <Rtypes.h>
#include <TMath.h>

#include <sstream>
#include <iostream>
#include <functional>
#include <map>
#include <vector>
#include <string>

class Plotter1D : public Plotter{
    
    private:
        std::map<std::string, std::vector<TH1F*>> background;
        std::map<std::string, std::vector<TH1F*>> signal;
        std::map<std::string, TH1F*> data;

        std::vector<std::string> parameters;

        std::string channel;
        std::vector<std::string> processes;


    public:
        Plotter1D();
        Plotter1D(std::string &histdir, std::string &channel, std::vector<std::string> &processes);
        void ConfigureHists();
        void Draw(std::vector<std::string> &outdirs);
        
};

#endif
