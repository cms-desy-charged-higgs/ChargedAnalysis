#ifndef PLOTTER2D_H
#define PLOTTER2D_H

#include <ChargedAnalysis/Analysis/include/plotter.h>

#include <TCanvas.h>
#include <TFile.h>
#include <TH2F.h>

class Plotter2D : public Plotter{
    
    private:
        std::vector<std::vector<std::vector<TH2F*>>> background;
        std::vector<std::vector<std::vector<TH2F*>>> signal;
        std::vector<TH2F*> data;

    public:
        Plotter2D();
        Plotter2D(std::string &histdir, std::vector<std::string> &xParameters, std::vector<std::string> &yParameters, std::string &channel);
        void ConfigureHists(std::vector<std::string> &processes);
        void Draw(std::vector<std::string> &outdirs);
        
};

#endif
