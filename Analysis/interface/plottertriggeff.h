#ifndef PLOTTERTRIGGEFF_H
#define PLOTTERTRIGGEFF_H

#include <ChargedHiggs/Analysis/interface/plotter.h>

#include <TH1.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TFile.h>
#include <TCanvas.h>
#include <TPad.h>
#include <TLegend.h>
#include <TGraphAsymmErrors.h>
#include <TEfficiency.h>

class PlotterTriggEff : public Plotter{
    private:
        std::string total;
        std::vector<std::string> passed;
        std::vector<std::string> yParam;

        std::map<std::string, std::string> ptNames;
        std::map<std::string, std::string> phiNames;
        std::map<std::string, std::string> etaNames;

        std::vector<std::vector<TGraphAsymmErrors*>> bkgEfficiencies;
        std::vector<std::vector<TGraphAsymmErrors*>> dataEfficiencies;

    public:
        PlotterTriggEff();
        PlotterTriggEff(std::string &histdir, std::string &total, std::vector<std::string> &passed, std::vector<std::string> &yParam, std::string &channel);

        void ConfigureHists(std::vector<std::string> &processes);
        void Draw(std::vector<std::string> &outdirs);
        
};

#endif
