#ifndef PLOTTER2D_H
#define PLOTTER2D_H

#include <TCanvas.h>
#include <TFile.h>
#include <TH2F.h>

#include <ChargedAnalysis/Analysis/include/plotter.h>
#include <ChargedAnalysis/Utility/include/utils.h>

class Plotter2D : public Plotter{
    
    private:
        std::vector<std::vector<std::vector<TH2F*>>> background;
        std::vector<std::vector<std::vector<TH2F*>>> signal;
        std::vector<TH2F*> data;

        std::string channel;
        std::vector<std::string> xParameters;
        std::vector<std::string> yParameters;
        std::vector<std::string> processes;

    public:
        Plotter2D();
        Plotter2D(std::string &histdir, std::vector<std::string> &xParameters, std::vector<std::string> &yParameters, std::string &channel, std::vector<std::string>& processes);
        void ConfigureHists();
        void Draw(std::vector<std::string> &outdirs);
        
};

#endif
