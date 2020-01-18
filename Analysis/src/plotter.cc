#include <ChargedAnalysis/Analysis/include/plotter.h>

Plotter::Plotter() : Plotter("") {}

Plotter::Plotter(const std::string& histdir):
    histdir(histdir),
    procDic({
            {"DY+j", BKG},
            {"W+j", BKG},
            {"SingleE", DATA},
            {"SingleMu", DATA},
            {"MET", DATA},
            {"VV+VVV", BKG},
            {"QCD", BKG},
            {"TT+j-1L", BKG},
            {"TT+j-2L", BKG},
            {"TT+j-H", BKG},
            {"TT+V", BKG},
            {"T", BKG},
            {"HPlus200_h100_4B", SIGNAL},
            {"HPlus250_h100_4B", SIGNAL},
            {"HPlus300_h100_4B", SIGNAL},
            {"HPlus350_h100_4B", SIGNAL},
            {"HPlus400_h100_4B", SIGNAL},
            {"HPlus450_h100_4B", SIGNAL},
            {"HPlus500_h100_4B", SIGNAL},
            {"HPlus550_h100_4B", SIGNAL},
            {"HPlus600_h100_4B", SIGNAL},
    }),
    channelHeader({
            {"e4j", "e + 4j"},
            {"mu4j", "#mu + 4j"},
            {"e2j1fj", "e + 2j + 1fj"},
            {"mu2j1fj", "#mu + 2j + 1fj"},
            {"e2fj", "e + 2fj"},
            {"mu2fj", "#mu + 2fj"},
    }),
    colors({
        {"DY+j", kRed + -7}, 
        {"TT+j-1L", kYellow -7}, 
        {"TT+j-2L", kYellow +4}, 
        {"TT+j-H", kYellow + 7},
        {"TT+V", kOrange +2},            
        {"T", kGreen  + 2},             
        {"W+j", kCyan + 2},             
        {"QCD", kBlue -3},             
        {"VV+VVV", kViolet -3},
    })
    {}

void Plotter::SetStyle(){
    //Style options
    TGaxis::SetExponentOffset(-0.05, 0.005, "y");
    TGaxis::SetMaxDigits(3);
    gStyle->SetLegendBorderSize(0);
    gStyle->SetFillStyle(0);
    gStyle->SetOptStat(0);
    gStyle->SetOptTitle(0);
    gROOT->SetBatch();
    gErrorIgnoreLevel = kWarning;
}

void Plotter::SetPad(TPad* pad){
    //Pad options
    pad->SetLeftMargin(0.15);
    pad->SetRightMargin(0.1);
    pad->SetBottomMargin(0.12); 
}

void Plotter::SetHist(TH1* frameHist){
    //Hist options
    frameHist->GetXaxis()->SetTitleSize(0.05);
    frameHist->GetYaxis()->SetTitleSize(0.05);
    frameHist->GetXaxis()->SetTitleOffset(1.1);
    frameHist->GetXaxis()->SetLabelSize(0.05);
    frameHist->GetYaxis()->SetLabelSize(0.05);
}

void Plotter::DrawHeader(const bool &twoPads, const std::string &titleText, const std::string &cmsText){
    //CMS Work in Progres and Lumi information
    TLatex* channel_title = new TLatex();
    channel_title->SetTextFont(42);
    channel_title->SetTextSize(twoPads ? 0.045 : 0.035);

    TLatex* lumi = new TLatex();
    lumi->SetTextFont(42);
    lumi->SetTextSize(twoPads ? 0.045 : 0.035);

    TLatex* cms = new TLatex();
    cms->SetTextFont(62);
    cms->SetTextSize(twoPads ? 0.05 : 0.040);

    TLatex* work = new TLatex();
    work->SetTextFont(52);
    work->SetTextSize(twoPads ? 0.045 : 0.035);

    channel_title->DrawLatexNDC(0.17, 0.905, titleText.c_str());
    cms->DrawLatexNDC(twoPads ? 0.32: 0.33, 0.905, "CMS");
    work->DrawLatexNDC(twoPads ? 0.388 : 0.40, 0.905, cmsText.c_str());
    lumi->DrawLatexNDC(twoPads ? 0.65: 0.63, 0.905, "41.4 fb^{-1} (2017, 13 TeV)");
}
