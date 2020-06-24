#ifndef PLOTTERLIMIT_H
#define PLOTTERLIMIT_H

#include <vector>
#include <map>

#include <TFile.h>
#include <TGraph.h>
#include <TGraphAsymmErrors.h>
#include <TTree.h>
#include <TBranch.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TPad.h>
#include <TLeaf.h>

#include <ChargedAnalysis/Utility/include/utils.h>
#include <ChargedAnalysis/Analysis/include/plotter.h>

class PlotterLimit : public Plotter{
    private:
        std::string limitDir;
        std::vector<int> masses;
        std::vector<std::string> channels;

        std::map<int, float> xSecs;

        TGraph* theory;
        std::map<std::string, TGraph*> expected;
        std::map<std::string, TGraphAsymmErrors*> sigmaOne;
        std::map<std::string, TGraphAsymmErrors*> sigmaTwo;

    public:
        PlotterLimit();
        PlotterLimit(std::string &limitDir, const std::vector<int>& masses, const std::vector<std::string>& channels);
        void ConfigureHists();
        void Draw(std::vector<std::string> &outdirs);
};

#endif
