#ifndef PLOTTERPOSTFIT_H
#define PLOTTERPOSTFIT_H

#include <ChargedAnalysis/Analysis/include/plotter.h>

#include <vector>
#include <map>

#include <TROOT.h>
#include <TFile.h>
#include <THStack.h>
#include <TH1F.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TPad.h>
#include <TLegend.h>

class PlotterPostfit : public Plotter{
    private:
        std::string limitDir;
        int mass; 
        std::vector<std::string> channel;
        int max = 0;

        std::map<std::string, TH1F*> errorBand;
        std::map<std::string, std::vector<TH1F*>> backgrounds;
        std::map<std::string, TH1F*> signals;

        std::map<std::string, std::string> chanToDir;

    public:
        PlotterPostfit();
        PlotterPostfit(std::string &limitDir, int &mass, std::vector<std::string> &channel);
        void ConfigureHists(std::vector<std::string> &processes);
        void Draw(std::vector<std::string> &outdirs);
};

#endif
