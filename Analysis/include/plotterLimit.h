#ifndef PLOTTERLIMIT_H
#define PLOTTERLIMIT_H

#include <ChargedAnalysis/Analysis/include/plotter.h>

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
#include <TTreeReader.h>
#include <TTreeReaderValue.h>

class PlotterLimit : public Plotter{
    private:
        std::string limitDir;
        std::vector<int> masses; 

        std::map<int, float> xSecs;

        TGraph* theory;
        TGraph* expected;
        TGraphAsymmErrors* sigmaOne;
        TGraphAsymmErrors* sigmaTwo;


    public:
        PlotterLimit();
        PlotterLimit(std::string &limitDir, std::vector<int> masses);
        void ConfigureHists();
        void Draw(std::vector<std::string> &outdirs);
};

#endif
