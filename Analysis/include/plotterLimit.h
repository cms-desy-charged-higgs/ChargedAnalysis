#ifndef PLOTTERLIMIT_H
#define PLOTTERLIMIT_H

#include <vector>
#include <memory>
#include <map>

#include <TFile.h>
#include <TH2D.h>
#include <TH1D.h>
#include <TGraph.h>
#include <TGraphAsymmErrors.h>
#include <TTree.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TPad.h>
#include <TLeaf.h>

#include <ChargedAnalysis/Utility/include/stringutil.h>
#include <ChargedAnalysis/Utility/include/vectorutil.h>
#include <ChargedAnalysis/Utility/include/plotutil.h>
#include <ChargedAnalysis/Utility/include/mathutil.h>
#include <ChargedAnalysis/Analysis/include/plotter.h>

class PlotterLimit : public Plotter{
    private:
        std::string limitDir;
        std::vector<int> chargedMasses, neutralMasses;
        std::vector<std::string> channels;
        std::string era;

        std::vector<float> xSecs;

        std::map<std::string, std::shared_ptr<TH2D>> expected;
        std::shared_ptr<TH2D> theory;
        std::map<std::pair<std::string, int>, std::shared_ptr<TGraphAsymmErrors>> sigmaOne;
        std::map<std::pair<std::string, int>, std::shared_ptr<TGraphAsymmErrors>> sigmaTwo;

    public:
        PlotterLimit();
        PlotterLimit(std::string &limitDir, const std::vector<int>& chargedMasses, const std::vector<int>& neutralMasses, const std::vector<std::string>& channels, const std::string& era, const std::vector<float>& xSecs);
        void ConfigureHists();
        void Draw(std::vector<std::string> &outdirs);
};

#endif
