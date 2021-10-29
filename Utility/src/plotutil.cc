#include <ChargedAnalysis/Utility/include/plotutil.h>

void PUtil::SetStyle(){
    //Style options
    TGaxis::SetExponentOffset(-0.07, 0.0035, "y");
    gStyle->SetLegendBorderSize(0);
    gStyle->SetFillStyle(0);
    gStyle->SetOptStat(0);
    gStyle->SetOptTitle(0);
    gROOT->SetBatch();
    gErrorIgnoreLevel = kWarning;
}

void PUtil::SetPad(TPad* pad, const bool& isRatio){
    float bottonMargin = 120./(pad->GetWw() * pad->GetWNDC());
    float leftMargin = 160./(pad->GetWw() * pad->GetWNDC());
    float rightMargin = 80./(pad->GetWw() * pad->GetWNDC());

    //Pad options
    pad->SetLeftMargin(leftMargin);
    pad->SetRightMargin(rightMargin);
    if(!isRatio) pad->SetBottomMargin(bottonMargin); 
}

void PUtil::SetHist(TPad* pad, TH1* frameHist, const std::string& yLabel, const bool& isRatio){
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

    if(!frameHist->InheritsFrom(TH2F::Class())) frameHist->GetYaxis()->SetTitle(yLabel.c_str());
    frameHist->GetYaxis()->SetTitleSize(labelSize/padHeight);
    frameHist->GetYaxis()->SetLabelOffset(labelDistance/padWidth);
    frameHist->GetYaxis()->SetLabelSize(labelSize/padHeight);
    frameHist->GetYaxis()->SetTickLength(tickSize/padWidth);
    frameHist->GetYaxis()->SetMaxDigits(3);
    if(isRatio) frameHist->GetYaxis()->SetNdivisions(3, 6, 0);
    if(isRatio) frameHist->GetYaxis()->SetTitleOffset(0.42);
}

void PUtil::DrawHeader(TPad* pad, const std::string& titleText, const std::string& cmsText, const std::string& lumiText){
    pad->cd();

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
    lumiLine->DrawLatexNDC(0.64, 0.91, lumiText.c_str());
}

TPad* PUtil::DrawRatio(TCanvas* canvas, TPad* mainPad, TH1F* num, TH1F* dem, const std::string& yLabel){
    //Resize Canvas
    canvas->cd();
    canvas->SetCanvasSize(canvas->GetWindowWidth(), 1.2*canvas->GetWindowHeight());
    mainPad->SetPad(0., 0.2, 1., 1.);

    //Set up ratio pad
    TPad* ratioPad = new TPad("ratioPad", "ratioPad", 0., 0. , 1., 0.2);
    PUtil::SetPad(ratioPad, true);

    ratioPad->Draw();
    ratioPad->cd();

    //Draw ratio histogram
    TH1F* ratio = (TH1F*)num->Clone();
    PUtil::SetHist(ratioPad, ratio, yLabel, true);
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

    return ratioPad;
}

TPad* PUtil::DrawLegend(TPad* pad, TLegend* legend, const int& nColumns){
    pad->cd();
        
    float max = 0;

    for(TObject* hist : *(pad->GetListOfPrimitives())){
        float maxHist = hist->InheritsFrom(TH1::Class()) ? static_cast<TH1*>(hist)->GetMaximum() : hist->InheritsFrom(TGraph::Class()) or hist->InheritsFrom(TGraphAsymmErrors::Class()) ? static_cast<TGraph*>(hist)->GetHistogram()->GetMaximum() : 0;

        max = max < maxHist ? maxHist : max;
    }

    float leftMargin = pad->GetLeftMargin();
    float rightMargin = pad->GetRightMargin();
    float topMargin = pad->GetTopMargin();

    TObject* frame = pad->GetListOfPrimitives()->At(0);
    int nRows = std::ceil(legend->GetListOfPrimitives()->GetSize()/float(nColumns));

    if(frame->InheritsFrom(TH1::Class())){
        static_cast<TH1*>(frame)->SetMaximum(pad->GetLogy() ? max*std::pow(10, nRows) : max*(1 + nRows*0.2));
    }

    else if(frame->InheritsFrom(TGraph::Class())){
        static_cast<TGraph*>(frame)->GetHistogram()->SetMaximum(pad->GetLogy() ? max*std::pow(10, nRows) : max*(1 + nRows*0.2));
    }

    //Draw Legend and legend pad
    TPad* legendPad = new TPad("legendPad", "legendPad", 1.05*leftMargin, 1-1.05*topMargin-nRows*0.07, 1-1.05*rightMargin, 1-1.05*topMargin);
    legendPad->Draw();
    legendPad->cd();

    float padWidth = legendPad->GetWw() * legendPad->GetWNDC();
    float padHeight = legendPad->GetWh() * legendPad->GetHNDC();

    float textSize = padHeight > padWidth ? 20./padWidth : 20./padHeight;

    for(int i=0; i < legend->GetListOfPrimitives()->GetSize(); ++i){
        ((TLegendEntry*)legend->GetListOfPrimitives()->At(i))->SetTextSize(textSize);
    }

    legend->SetNColumns(nColumns);
    legend->Draw();

    return legendPad;
}

void PUtil::DrawShapes(TCanvas* canvas, TH1* bkg, TH1* sig){
    TH1* s = (TH1*)sig->Clone(); s->Scale(1./s->Integral());
    TH1* b = nullptr;
    if(bkg != nullptr){
        b = (TH1*)bkg->Clone(); b->Scale(1./b->Integral());
    }

    TLegend* l = new TLegend(0., 0., 1, 1);

    PUtil::SetPad(canvas);
    PUtil::SetHist(canvas, s, "Normalized events");

    canvas->Draw();

    //Draw Legend and legend pad
    s->SetLineColor(kBlue+1);
    s->SetFillStyle(3335);
    s->SetFillColor(kBlue);
    s->SetLineWidth(4);
    l->AddEntry(s, "Sig", "F");

    s->Draw("HIST");

    if(b != nullptr){
        b->SetLineColor(kRed+1);
        b->SetFillStyle(3353);
        b->SetFillColor(kRed);
        b->SetLineWidth(4);
        l->AddEntry(b, "Bkg", "F");

        b->Draw("HIST SAME");
    }

    PUtil::DrawLegend(canvas, l, 2);
}

float PUtil::DrawConfusion(const std::vector<long>& trueLabel, const std::vector<long>& predLabel, const std::vector<std::string>& classNames, const std::vector<std::string>& outDirs, const bool& isVali){
    for(const std::string outDir : outDirs){
        if(!std::filesystem::exists(outDir)){
            std::filesystem::create_directories(outDir);
        }
    }

    int nClasses = classNames.size();
    float accuracy = 0.;

    //Set canvas/hist
    std::shared_ptr<TCanvas> canvas = std::make_shared<TCanvas>("c", "c", 1000, 1000);
    std::shared_ptr<TH2F> confusion = std::make_shared<TH2F>("h", "h", nClasses, 0, nClasses, nClasses, 0, nClasses);
    std::shared_ptr<TH2F> confusionNormed = RUtil::CloneSmart<TH2F>(confusion.get());
    
    //Set alphanumeric labels
    for(int i = 0; i < classNames.size(); ++i){
        confusionNormed->GetXaxis()->SetBinLabel(i + 1, classNames.at(i).c_str()); 
        confusionNormed->GetYaxis()->SetBinLabel(classNames.size() - i, classNames.at(i).c_str()); 
    }

    //Style stuff
    PUtil::SetStyle();
    PUtil::SetPad(canvas.get());
    PUtil::SetHist(canvas.get(), confusionNormed.get(), "");

    gStyle->SetPaintTextFormat("4.3f");
    confusionNormed->SetMarkerSize(2);
    confusionNormed->GetXaxis()->SetTitle("Predicted label");
    confusionNormed->GetYaxis()->SetTitle("True label");
    
    //Fill confusion matrix
    for(int i = 0; i < trueLabel.size(); ++i){
        confusion->Fill(predLabel.at(i), trueLabel.at(i));
    }
    
    //Normalize
    for(int i = 0; i < nClasses; ++i){
        float nTotal = 0.;
        
        for(int j = 0; j < nClasses; ++j){
            nTotal += confusion->GetBinContent(j + 1, nClasses - i);
        }
        
        for(int j = 0; j < nClasses; ++j){
            confusionNormed->SetBinContent(j + 1, i + 1, confusion->GetBinContent(j + 1, nClasses - i)/nTotal);
        }
    }

    //Calculate accuracy
    for(int i = 0; i < nClasses; ++i){
        if(!std::isnan(confusionNormed->GetBinContent(i + 1, nClasses - i)))
        accuracy += confusionNormed->GetBinContent(i + 1, nClasses - i)/float(nClasses);
    }
    
    //Draw and save
    confusionNormed->Draw("COL");
    confusionNormed->Draw("TEXT SAME");
    
    for(const std::string outDir : outDirs){
        canvas->SaveAs((outDir + "/confusion" + (isVali ? "_vali" : "_train") + ".pdf").c_str());
        canvas->SaveAs((outDir + "/confusion" + (isVali ? "_vali" : "_train") + ".png").c_str());
    }

    return accuracy;
}

void PUtil::DrawLoss(const std::vector<std::string> outDirs, const float& trainLoss, const float& valLoss, const float& trainAcc, const float& valAcc){
    for(const std::string outDir : outDirs){
        if(!std::filesystem::exists(outDir)){
            std::filesystem::create_directories(outDir);
        }
    }

    std::shared_ptr<TFile> fStatsNew, fStatsOld;
    std::shared_ptr<TGraph> vLoss, tLoss, vAcc, tAcc; 

    fStatsNew = std::make_shared<TFile>((outDirs.at(0) + "/tmp.root").c_str(), "RECREATE");

    if(std::filesystem::exists(outDirs.at(0) + "/stats.root")){
        fStatsOld = RUtil::Open(outDirs.at(0) + "/stats.root");
        
        vLoss = RUtil::CloneSmart(RUtil::Get<TGraph>(fStatsOld.get(), "valLoss"));
        tLoss = RUtil::CloneSmart(RUtil::Get<TGraph>(fStatsOld.get(), "trainLoss"));
        vAcc = RUtil::CloneSmart(RUtil::Get<TGraph>(fStatsOld.get(), "valiAcc"));
        tAcc = RUtil::CloneSmart(RUtil::Get<TGraph>(fStatsOld.get(), "trainAcc"));
    }
    
    else{        
        vLoss = std::make_shared<TGraph>();
        vLoss->SetTitle("valLoss");
        vLoss->SetName("valLoss");
        
        tLoss = std::make_shared<TGraph>();
        tLoss->SetTitle("trainLoss");
        tLoss->SetName("trainLoss");

        vAcc = std::make_shared<TGraph>();
        vAcc->SetTitle("valiAcc");
        vAcc->SetName("valiAcc");
        
        tAcc = std::make_shared<TGraph>();
        tAcc->SetTitle("trainAcc");
        tAcc->SetName("trainAcc");
    }
    
    fStatsNew = std::make_shared<TFile>((outDirs.at(0) + "/tmp.root").c_str(), "RECREATE");

    vLoss->SetPoint(vLoss->GetN(), vLoss->GetN(), valLoss);
    tLoss->SetPoint(tLoss->GetN(), tLoss->GetN(), trainLoss);
    vAcc->SetPoint(vAcc->GetN(), vAcc->GetN(), valAcc);
    tAcc->SetPoint(tAcc->GetN(), tAcc->GetN(), trainAcc);

    vLoss->SetLineWidth(5);
    vLoss->SetLineColor(kBlue);

    tLoss->SetLineWidth(5);
    tLoss->SetLineColor(kViolet);

    vAcc->SetLineWidth(5);
    vAcc->SetLineColor(kBlack);

    tAcc->SetLineWidth(5);
    tAcc->SetLineColor(kViolet);

    std::shared_ptr<TLegend> l = std::make_shared<TLegend>(0., 0., 1, 1);
    l->AddEntry(vLoss.get(), "Validation", "L");
    l->AddEntry(tLoss.get(), "Train", "L");

    std::shared_ptr<TLegend> l2 = std::make_shared<TLegend>(0., 0., 1, 1);
    l2->AddEntry(vAcc.get(), "Validation", "L");
    l2->AddEntry(tAcc.get(), "Train", "L");

    std::shared_ptr<TCanvas> canvas = std::make_shared<TCanvas>("c", "c", 1000, 1000);
    PUtil::SetStyle();
    PUtil::SetPad(canvas.get());
    PUtil::SetHist(canvas.get(), vLoss->GetHistogram(), "");
    vLoss->GetHistogram()->GetXaxis()->SetTitle("Epoch");
    vLoss->GetHistogram()->GetYaxis()->SetTitle("Loss");

    vLoss->Draw("AL");
    tLoss->Draw("L SAME");

    PUtil::DrawLegend(canvas.get(), l.get(), 3);

    for(const std::string outDir : outDirs){
        canvas->SaveAs((outDir + "/loss.pdf").c_str());
        canvas->SaveAs((outDir + "/loss.png").c_str());
    }

    canvas->Clear();

    PUtil::SetHist(canvas.get(), vAcc->GetHistogram(), "");
    vAcc->GetHistogram()->GetXaxis()->SetTitle("Epoch");
    vAcc->GetHistogram()->GetYaxis()->SetTitle("Accuracy");

    vAcc->Draw("AL");
    tAcc->Draw("L SAME");

    PUtil::DrawLegend(canvas.get(), l2.get(), 3);

    for(const std::string outDir : outDirs){
        canvas->SaveAs((outDir + "/accuracy.pdf").c_str());
        canvas->SaveAs((outDir + "/accuracy.png").c_str());
    }
   
    fStatsNew->cd();
    vLoss->Write();
    tLoss->Write();
    vAcc->Write();
    tAcc->Write();
  
    fStatsNew->Close();
    if(fStatsOld) fStatsOld->Close();
    
    std::filesystem::rename(outDirs.at(0) + "/tmp.root", outDirs.at(0) + "/stats.root");
}

std::string PUtil::GetChannelTitle(const std::string& channel){
    std::map<std::string, std::string> channelTitle = {
        {"EleIncl", "e incl."},
        {"MuonIncl", "#mu incl."},
        {"Ele4J", "e + 4j"},
        {"Muon4J", "#mu + 4j"},
        {"Ele2J1FJ", "e + 2j + 1fj"},
        {"Muon2J1FJ", "#mu + 2j + 1fj"},
        {"Ele2FJ", "e + 2fj"},
        {"Muon2FJ", "#mu + 2fj"},
    };

    return VUtil::At(channelTitle, channel);
}

std::string PUtil::GetLumiTitle(const std::string& lumi){
    std::map<std::string, std::string> lumiTitle = {
        {"2016Pre", "19.52 fb^{-1} (2016 pre-VFP)"}, 
        {"2016Post", "16.81 fb^{-1} (2016 post-VFP)"}, 
        {"2017", "41.53 fb^{-1} (2017)"}, 
        {"2018", "59.74 fb^{-1} (2018)"},
        {"RunII", "137.19 fb^{-1} (RunII)"},
    };

    return VUtil::At(lumiTitle, lumi);
}

int PUtil::GetProcColor(const std::string& proc){
    std::map<std::string, int> colors = {
        {"DYJ", kRed + -7}, 
        {"TT1L", kYellow -7}, 
        {"TT2L", kYellow +4}, 
        {"TTHad", kYellow +8}, 
        {"TTV", kOrange +2},            
        {"ST", kGreen  + 2},             
        {"WJ", kCyan + 2},             
        {"QCD", kBlue -3},
        {"MisIDJ", kBlue -5},
        {"VV", kViolet -3},
    };

    if(!colors.count(proc)) throw std::runtime_error("Unknown color for given process: " + proc);

    return colors.at(proc);
}


std::string PUtil::GetProcTitle(const std::string& proc){
    std::map<std::string, std::string> titles = {
        {"DYJ", "Z#rightarrowll"}, 
        {"TT1L", "t#bar{t}#rightarrow l+X"}, 
        {"TT2L", "t#bar{t}#rightarrow 2l+X"}, 
        {"TTV", "t#bar{t}+W/Z"},            
        {"ST", "t/tW#rightarrow l+X"},             
        {"WJ", "W#rightarrowl#nu"},             
        {"QCD", "QCD"},
        {"MisIDJ", "Misid. jet"},
        {"VV", "ZZ/WZ/WW"},
    };

    if(!StrUtil::Find(proc, "HPlus").empty()){
        std::vector<std::string> s = StrUtil::Split(proc, "_");
        return StrUtil::Replace("H^{#pm}_{[M]} + h_{[M]}", "[M]", s.at(0).substr(5), s.at(1).substr(1));    
    }

    else if(!titles.count(proc)) throw std::runtime_error("Unknown title for given process: " + proc);

    return titles.at(proc);
}
