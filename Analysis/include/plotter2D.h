#ifndef PLOTTER2D_H
#define PLOTTER2D_H

#include <TCanvas.h>
#include <TFile.h>
#include <TH2F.h>

#include <ChargedAnalysis/Analysis/include/plotter.h>
#include <ChargedAnalysis/Utility/include/utils.h>

class Plotter2D : public Plotter{
    
    private:
        std::map<std::string, std::vector<TH2F*>> background;
        std::map<std::string, std::vector<TH2F*>> signal;
        std::map<std::string, TH2F*> data;

        std::string channel;
        std::vector<std::string> processes;
        std::vector<std::string> parameters;
        std::string era;

    public:
        Plotter2D();
        Plotter2D(std::string &histdir, std::string &channel, std::vector<std::string> &processes, const std::string& era);
        void ConfigureHists();
        void Draw(std::vector<std::string> &outdirs);
        
};

#endif
