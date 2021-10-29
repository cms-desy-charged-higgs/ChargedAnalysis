#ifndef PLOTTERPOSTFIT_H
#define PLOTTERPOSTFIT_H

#include <functional>
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
#include <TGraphAsymmErrors.h>

#include <ChargedAnalysis/Utility/include/stringutil.h>
#include <ChargedAnalysis/Utility/include/plotutil.h>
#include <ChargedAnalysis/Utility/include/rootutil.h>

class PlotterPostfit{
    private:
        std::string inFile, sigProcess;
        std::vector<std::string> bkgProcesses;
        int max = 0;
        bool isPostfit;

        std::map<std::string, std::shared_ptr<TH1F>> backgrounds;
        std::shared_ptr<TH1F> errorBand;
        std::shared_ptr<TH1F> signal;
        std::shared_ptr<TGraphAsymmErrors> data;


    public:
        PlotterPostfit();
        PlotterPostfit(const std::string& inFile, const std::vector<std::string>& bkgProcesses, const std::string& sigProcess, const bool& isPostfit = true);
        void ConfigureHists();
        void Draw(std::vector<std::string> &outdirs);
};

#endif
