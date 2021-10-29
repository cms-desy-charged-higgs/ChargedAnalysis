#ifndef PLOTUTIL_H
#define PLOTUTIL_H

#include <string>
#include <filesystem>

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
#include <TGraphAsymmErrors.h>

#include <ChargedAnalysis/Utility/include/vectorutil.h>
#include <ChargedAnalysis/Utility/include/rootutil.h>

namespace PUtil{
    void SetStyle();
    void SetPad(TPad* pad, const bool& isRatio=false);
    void SetHist(TPad* pad, TH1* frameHist, const std::string& yLabel="", const bool& isRatio=false);

    void DrawHeader(TPad* pad, const std::string& titleText, const std::string& cmsText, const std::string& lumiText = "");
    TPad* DrawRatio(TCanvas* canvas, TPad* mainPad, TH1F* num, TH1F* dem, const std::string& yLabel="");
    TPad* DrawLegend(TPad* pad, TLegend* legend, const int& nColumns);
    void DrawShapes(TCanvas* canvas, TH1* bkg, TH1* sig);
    
    float DrawConfusion(const std::vector<long>& trueLabel, const std::vector<long>& predLabel, const std::vector<std::string>& classNames, const std::vector<std::string>& outDirs, const bool& isVali);
    void DrawLoss(const std::vector<std::string> outDirs, const float& trainLoss, const float& valLoss,  const float& trainAcc, const float& valAcc);

    std::string GetChannelTitle(const std::string& channel);
    std::string GetLumiTitle(const std::string& lumi);
    int GetProcColor(const std::string& proc);
    std::string GetProcTitle(const std::string& proc);
};

#endif
