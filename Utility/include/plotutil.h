#ifndef PLOTUTIL_H
#define PLOTUTIL_H

#include <string>

#include <TPad.h>
#include <TCanvas.h>
#include <TH1.h>
#include <TH2F.h>
#include <TGraph.h>
#include <TGaxis.h>
#include <TError.h>
#include <TLegend.h>
#include <TLegendEntry.h>
#include <TROOT.h>
#include <TStyle.h>
#include <TLatex.h>

#include <ChargedAnalysis/Utility/include/vectorutil.h>

namespace PUtil{
    void SetStyle();
    void SetPad(TPad* pad, const bool& isRatio=false);
    void SetHist(TPad* pad, TH1* frameHist, const std::string& yLabel="", const bool& isRatio=false);

    void DrawHeader(TPad* pad, const std::string& titleText, const std::string& cmsText, const std::string& lumiText = "");
    void DrawRatio(TCanvas* canvas, TPad* mainPad, TH1F* num, TH1F* dem, const std::string& yLabel="");
    void DrawLegend(TPad* pad, TLegend* legend, const int& nColumns);
    void DrawShapes(TCanvas* canvas, TH1* bkg, TH1* sig);

    std::string GetChannelTitle(const std::string& channel);
    std::string GetLumiTitle(const std::string& lumi);
};

#endif
