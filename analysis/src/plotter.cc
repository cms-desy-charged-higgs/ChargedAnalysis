#include <ChargedHiggs/analysis/interface/plotter.h>

Plotter::Plotter(){}

Plotter::~Plotter(){}

Plotter::Plotter(std::string &histdir, std::vector<std::string> &xParameters):
    histdir(histdir),
    xParameters(xParameters),
    yParameters({}),
    procDic({
            {"DY+j", BKG},
            {"W+j", BKG},
            {"SingleE", DATA},
            {"SingleMu", DATA},
            {"VV+VVV", BKG},
            {"QCD", BKG},
            {"TT+X", BKG},
            {"T+X", BKG},
            {"L4B_150_75", SIGNAL},
            {"L4B_200_100", SIGNAL},
            {"L4B_600_100", SIGNAL},
    })
{}


Plotter::Plotter(std::string &histdir, std::vector<std::string> &xParameters, std::vector<std::string> &yParameters):
    histdir(histdir),
    xParameters(xParameters),
    yParameters(yParameters),
    procDic({
            {"DY+j", BKG},
            {"W+j", BKG},
            {"SingleE", DATA},
            {"SingleMu", DATA},
            {"VV+VVV", BKG},
            {"QCD", BKG},
            {"TT+X", BKG},
            {"T+X", BKG},
            {"L4B_150_75", SIGNAL},
            {"L4B_200_100", SIGNAL},
            {"L4B_600_100", SIGNAL},
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
