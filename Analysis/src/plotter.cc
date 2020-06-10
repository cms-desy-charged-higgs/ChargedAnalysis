#include <ChargedAnalysis/Analysis/include/plotter.h>

Plotter::Plotter() : Plotter("") {}

Plotter::Plotter(const std::string& histdir):
    histdir(histdir),
    channelHeader({
            {"Ele4J", "e + 4j"},
            {"Muon4J", "#mu + 4j"},
            {"Ele2J1FJ", "e + 2j + 1fj"},
            {"Muon2J1FJ", "#mu + 2j + 1fj"},
            {"Ele2FJ", "e + 2fj"},
            {"Muon2FJ", "#mu + 2fj"},
    }),
    colors({
        {"DY+j", kRed + -7}, 
        {"DYqq", kRed + -4}, 
        {"TT-1L", kYellow -7}, 
        {"TT-2L", kYellow +4}, 
        {"TT-Had", kYellow +8}, 
        {"TT+V", kOrange +2},            
        {"T", kGreen  + 2},             
        {"W+j", kCyan + 2},             
        {"QCD", kBlue -3},             
        {"VV+VVV", kViolet -3},
    })
    {}

void Plotter::SetStyle(){
    //Style options
    TGaxis::SetExponentOffset(-0.07, 0.0035, "y");
    gStyle->SetLegendBorderSize(0);
    gStyle->SetFillStyle(0);
    gStyle->SetOptStat(0);
    gStyle->SetOptTitle(0);
    gROOT->SetBatch();
    gErrorIgnoreLevel = kWarning;
}

void Plotter::SetPad(TPad* pad, const bool& isRatio){
    float bottonMargin = 120./(pad->GetWw() * pad->GetWNDC());
    float leftMargin = 160./(pad->GetWw() * pad->GetWNDC());
    float rightMargin = 80./(pad->GetWw() * pad->GetWNDC());

    //Pad options
    pad->SetLeftMargin(leftMargin);
    pad->SetRightMargin(rightMargin);
    if(!isRatio) pad->SetBottomMargin(bottonMargin); 
}

void Plotter::SetHist(TPad* pad, TH1* frameHist, const std::string& yLabel, const bool& isRatio){
    float padWidth = pad->GetWw() * pad->GetWNDC();
    float padHeight = pad->GetWh() * pad->GetHNDC();

    float labelSize = 40.;
    float labelDistance = 10.;
    float tickSize = 40.;

    //Hist options
    frameHist->GetXaxis()->SetTitleSize(isRatio ? 0 : labelSize/padHeight);
    frameHist->GetXaxis()->SetTitleOffset(1. + 0.25);
    frameHist->GetXaxis()->SetLabelSize(isRatio ? 0 : labelSize/padHeight);
    frameHist->GetXaxis()->SetLabelOffset(isRatio ? 0 : labelDistance/padHeight);
    frameHist->GetXaxis()->SetTickLength(tickSize/padHeight);

    frameHist->GetYaxis()->SetTitle(yLabel.c_str());
    frameHist->GetYaxis()->SetTitleSize(labelSize/padHeight);
    frameHist->GetYaxis()->SetLabelOffset(labelDistance/padWidth);
    frameHist->GetYaxis()->SetLabelSize(labelSize/padHeight);
    frameHist->GetYaxis()->SetTickLength(tickSize/padWidth);
    frameHist->GetYaxis()->SetMaxDigits(3);
    if(isRatio) frameHist->GetYaxis()->SetNdivisions(3, 6, 0);
    if(isRatio) frameHist->GetYaxis()->SetTitleOffset(0.42);
}

void Plotter::DrawHeader(TPad* pad, const std::string& titleText, const std::string& cmsText){
    float padWidth = pad->GetWw() * pad->GetWNDC();
    float padHeight = pad->GetWh() * pad->GetHNDC();

    float textSize = padHeight > padWidth ? 30./padWidth : 30./padHeight;

    TLatex* channelLine = new TLatex();
    channelLine->SetTextFont(42);
    channelLine->SetTextSize(textSize);
    channelLine->DrawLatexNDC(0.17, 0.91, titleText.c_str());

    TLatex* cmsLine = new TLatex();
    cmsLine->SetTextFont(62);
    cmsLine->SetTextSize(textSize);
    cmsLine->DrawLatexNDC(0.33, 0.91, "CMS");

    TLatex* cmsTextLine = new TLatex();
    cmsTextLine->SetTextFont(52);
    cmsTextLine->SetTextSize(textSize);
    cmsTextLine->DrawLatexNDC(0.4, 0.91, cmsText.c_str());

    //CMS Work in Progres and Lumi information
    TLatex* lumiLine = new TLatex();
    lumiLine->SetTextFont(42);
    lumiLine->SetTextSize(textSize);
    lumiLine->DrawLatexNDC(0.64, 0.91, "41.4 fb^{-1} (2017, 13 TeV)");
}

void Plotter::DrawRatio(TCanvas* canvas, TPad* mainPad, TH1F* num, TH1F* dem, const std::string& yLabel){
    //Resize Canvas
    canvas->cd();
    canvas->SetCanvasSize(canvas->GetWindowWidth(), 1.2*canvas->GetWindowHeight());
    mainPad->SetPad(0., 0.2, 1., 1.);

    //Set up ratio pad
    TPad* ratioPad = new TPad("ratioPad", "ratioPad", 0., 0. , 1., 0.2);
    Plotter::SetPad(ratioPad, true);

    ratioPad->Draw();
    ratioPad->cd();

    //Draw ratio histogram
    TH1F* ratio = (TH1F*)num->Clone();
    Plotter::SetHist(ratioPad, ratio, yLabel, true);
    ratio->Divide(dem);
    ratio->SetMinimum(0.5);
    ratio->SetMaximum(1.5);
    ratio->Draw();

    //Draw uncertainty band
    TH1F* uncertainty = (TH1F*)dem->Clone();
    uncertainty->Divide(dem);
    uncertainty->SetFillStyle(3354);
    uncertainty->SetFillColorAlpha(kBlack, 0.8);
    uncertainty->SetMarkerColor(kBlack);
    uncertainty->Draw("SAME E2");
}

void Plotter::DrawLegend(TLegend* legend, const int& nColumns){
    //Draw Legend and legend pad
    TPad* legendPad = new TPad("legendPad", "legendPad", 0.18, 0.88-nColumns*0.02, 0.91, 0.88);
    legendPad->Draw();
    legendPad->cd();

    float padWidth = legendPad->GetWw() * legendPad->GetWNDC();
    float padHeight = legendPad->GetWh() * legendPad->GetHNDC();

    float textSize = padHeight > padWidth ? 30./padWidth : 30./padHeight;

    for(int i=0; i < legend->GetListOfPrimitives()->GetSize(); i++){
        ((TLegendEntry*)legend->GetListOfPrimitives()->At(i))->SetTextSize(textSize);
    }

    legend->SetNColumns(nColumns);
    legend->Draw();
}

void Plotter::DrawShapes(TCanvas* canvas, TH1* bkg, TH1* sig){
    TH1* b = (TH1*)bkg->Clone(); b->Scale(1./b->Integral());
    TH1* s = (TH1*)sig->Clone(); s->Scale(1./s->Integral());
    TLegend* l = new TLegend(0., 0., 1, 1);

    Plotter::SetPad(canvas);
    Plotter::SetHist(canvas, b, "Normalized events");

    canvas->Draw();

    float max = std::max({b->GetMaximum(), s->GetMaximum()});
    b->SetMaximum(max*(1 + 0.2));

    //Draw Legend and legend pad
    s->SetLineColor(kBlue+1);
    s->SetFillStyle(3335);
    s->SetFillColor(kBlue);
    s->SetLineWidth(4);

    b->SetLineColor(kRed+1);
    b->SetFillStyle(3353);
    b->SetFillColor(kRed);
    b->SetLineWidth(4);

    b->Draw("HIST");
    s->Draw("HIST SAME");

    l->AddEntry(b, "Bkg", "F");
    l->AddEntry(s, "Sig", "F");
    Plotter::DrawLegend(l, 2);
}
