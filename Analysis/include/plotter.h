#ifndef PLOTTER_H
#define PLOTTER_H

#include <string>
#include <map>
#include <vector>
#include <iostream>

#include <TLatex.h>
#include <TGaxis.h>
#include <TError.h>
#include <TROOT.h>
#include <TLegend.h>
#include <TLegendEntry.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TH1.h>
#include <TPad.h>

class Plotter{
    protected:
        std::map<std::string, std::string> channelHeader;
        std::map<std::string, int> colors;

        std::string histdir;

    public:
        Plotter();
        Plotter(const std::string& histdir);

        static void SetStyle();
        static void SetPad(TPad* pad, const bool& isRatio=false);
        static void SetHist(TPad* pad, TH1* frameHist, const std::string& yLabel="", const bool& isRatio=false);
        static void DrawHeader(TPad* pad, const std::string& titleText, const std::string& cmsText);
        static void DrawRatio(TCanvas* canvas, TPad* mainPad, TH1F* num, TH1F* dem, const std::string& yLabel="");
        static void DrawLegend(TLegend* legend, const int& nColumns);

        virtual void ConfigureHists() = 0;
        virtual void Draw(std::vector<std::string> &outdirs) = 0;
        
};

#endif
