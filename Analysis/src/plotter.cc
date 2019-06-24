#include <ChargedHiggs/analysis/interface/plotter.h>

Plotter::Plotter(){}

Plotter::~Plotter(){}

Plotter::Plotter(std::string &histdir, std::vector<std::string> &xParameters, std::string &channel):
    histdir(histdir),
    xParameters(xParameters),
    yParameters({}),
    channel(channel),
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
            {"TT+V", BKG},
            {"T", BKG},
            {"L4B_150_75", SIGNAL},
            {"L4B_200_100", SIGNAL},
            {"L4B_250_100", SIGNAL},
            {"L4B_300_100", SIGNAL},
            {"L4B_350_100", SIGNAL},
            {"L4B_400_100", SIGNAL},
            {"L4B_450_100", SIGNAL},
            {"L4B_500_100", SIGNAL},
            {"L4B_550_100", SIGNAL},
            {"L4B_600_100", SIGNAL},
    }),
    channelHeader({
            {"e4j", "e + 4j"},
            {"mu4j", "#mu + 4j"},
            {"e2j1f", "e + 2j + 1fj"},
            {"mu2j1f", "#mu + 2j + 1fj"},
            {"e2f", "e + 2fj + "},
            {"mu2f", "#mu + 2fj"},
    })
{}


Plotter::Plotter(std::string &histdir, std::vector<std::string> &xParameters, std::vector<std::string> &yParameters, std::string &channel):
    histdir(histdir),
    xParameters(xParameters),
    yParameters(yParameters),
    channel(channel),
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
            {"TT+V", BKG},
            {"T", BKG},
            {"L4B_150_75", SIGNAL},
            {"L4B_200_100", SIGNAL},
            {"L4B_250_100", SIGNAL},
            {"L4B_300_100", SIGNAL},
            {"L4B_350_100", SIGNAL},
            {"L4B_400_100", SIGNAL},
            {"L4B_450_100", SIGNAL},
            {"L4B_500_100", SIGNAL},
            {"L4B_550_100", SIGNAL},
            {"L4B_600_100", SIGNAL},
    }),
    channelHeader({
            {"e4j", "e + 4j"},
            {"mu4j", "#mu + 4j"},
            {"e2j1f", "e + 2j + 1fj"},
            {"mu2j1f", "#mu + 2j + 1fj"},
            {"e2f", "e + 2fj"},
            {"mu2f", "#mu + 2fj"},
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
