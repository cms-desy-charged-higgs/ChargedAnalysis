#include <ChargedAnalysis/Analysis/include/plotterLimit.h>

PlotterLimit::PlotterLimit(){}

PlotterLimit::PlotterLimit(const std::vector<std::string> limitFiles, const std::vector<int>& chargedMasses, const std::vector<int>& neutralMasses, const std::string& channel, const std::string& era, const std::vector<float>& xSecs) : 
    limitFiles(limitFiles),
    chargedMasses(chargedMasses),
    neutralMasses(neutralMasses),
    channel(channel),
    era(era),
    xSecs(xSecs){}

void PlotterLimit::ConfigureHists(){
    expected2D = std::make_shared<TGraph2D>();
    theory2D = std::make_shared<TGraph2D>();
    sigmaOneUp2D = std::make_shared<TGraph2D>();
    sigmaOneDown2D = std::make_shared<TGraph2D>();
    sigmaTwoUp2D = std::make_shared<TGraph2D>();
    sigmaTwoDown2D = std::make_shared<TGraph2D>();

    for(unsigned int i=0; i < chargedMasses.size(); ++i){
        int mHC = chargedMasses[i], mh = neutralMasses[i];

        std::shared_ptr<TFile> limitFile = RUtil::Open(limitFiles.at(i));
        std::shared_ptr<TTree> limitTree = RUtil::GetSmart<TTree>(limitFile.get(), "limit");

        //Set branch for reading limits
        std::vector<double> limitValues;
        TLeaf* limitValue = RUtil::Get<TLeaf>(limitTree.get(), "limit");       

        //Values
        for(int k=0; k < 5; k++){
            limitValues.push_back(RUtil::GetEntry<double>(limitValue, k) * xSecs.at(i));
        }

        expected2D->AddPoint(mHC, mh, limitValues[2]);    
        theory2D->AddPoint(mHC, mh, xSecs.at(i));

        if(!sigmaOne.count(mh)){
            theory[mh] = std::make_shared<TGraph>();
            expected[mh] = std::make_shared<TGraph>();
            sigmaOne[mh] = std::make_shared<TGraphAsymmErrors>();
            sigmaTwo[mh] = std::make_shared<TGraphAsymmErrors>();

            expected[mh]->SetLineWidth(3);
            expected[mh]->SetLineColor(kBlack);

            theory[mh]->SetLineWidth(3);
            theory[mh]->SetLineStyle(2);
            theory[mh]->SetLineColor(kBlack);

            sigmaOne[mh]->SetFillColor(kGreen);
            sigmaOne[mh]->SetFillStyle(1001);  

            sigmaTwo[mh]->SetFillColor(kYellow);
            sigmaTwo[mh]->SetFillStyle(1001);
        }

        theory[mh]->SetPoint(theory[mh]->GetN(), mHC, xSecs[i]);
        expected[mh]->SetPoint(expected[mh]->GetN(), mHC, limitValues[2]);

        sigmaOne[mh]->SetPoint(sigmaOne[mh]->GetN(), mHC, limitValues[2]);
        sigmaOne[mh]->SetPointEYlow(sigmaOne[mh]->GetN() - 1, limitValues[2] - limitValues[1]);
        sigmaOne[mh]->SetPointEYhigh(sigmaOne[mh]->GetN() - 1, limitValues[3] - limitValues[1]);  

        sigmaTwo[mh]->SetPoint(sigmaTwo[mh]->GetN(), mHC, limitValues[2]);
        sigmaTwo[mh]->SetPointEYlow(sigmaTwo[mh]->GetN() - 1, limitValues[2] - limitValues[0]);
        sigmaTwo[mh]->SetPointEYhigh(sigmaTwo[mh]->GetN() - 1, limitValues[4] - limitValues[1]);
    }
}

void PlotterLimit::Draw(std::vector<std::string>& outdirs){
    std::shared_ptr<TCanvas> canvas = std::make_shared<TCanvas>("canvas",  "canvas", 1000, 1000);
    std::shared_ptr<TPad> mainPad = std::make_shared<TPad>("mainPad", "mainPad", 0., 0. , 1., 1.);

    PUtil::SetStyle();

    //Draw main pad
    PUtil::SetPad(mainPad.get());
    mainPad->SetLogy(1);
    mainPad->Draw();
    mainPad->cd();

    //Plot charged Higgs 1D limit in slices of small higgs
    for(unsigned int j=0; j < neutralMasses.size(); j++){
        int mh = neutralMasses[j];

        //Frame histo
        std::shared_ptr<TH1F> frameHist = RUtil::CloneSmart(expected[mh]->GetHistogram());
        frameHist->Reset();
        PUtil::SetHist(mainPad.get(), frameHist.get(), "95% CL Limit on #sigma(pp #rightarrow H^{#pm}h #rightarrow lb#bar{b}b#bar{b}) [pb]");
  
        float min = std::min(theory[mh]->GetHistogram()->GetMinimum(), sigmaTwo[mh]->GetHistogram()->GetMinimum());

        frameHist->GetXaxis()->SetLimits(*std::min_element(chargedMasses.begin(), chargedMasses.end()),
                                         *std::max_element(chargedMasses.begin(), chargedMasses.end()));
        frameHist->SetMinimum(min*1e-1);
        frameHist->GetXaxis()->SetTitle("m(H^{#pm}) [GeV]");

        //Add legend information            
        std::shared_ptr<TLegend> legend = std::make_shared<TLegend>(0.0, 0.0, 1.0, 1.0);

        legend->AddEntry(expected[mh].get(), "Expected", "L");
        legend->AddEntry(theory[mh].get(), "Theo. prediction", "L");
        legend->AddEntry(sigmaOne[mh].get(), "68% expected", "F");
        legend->AddEntry(sigmaTwo[mh].get(), "95% expected", "F");

        //Draw everything and save
        frameHist->Draw();
        sigmaTwo[mh]->Draw("SAME 3");
        sigmaOne[mh]->Draw("SAME 3");
        expected[mh]->Draw("C SAME");
        theory[mh]->Draw("C SAME");
        gPad->RedrawAxis();
        PUtil::DrawHeader(mainPad.get(), channel == "Combined" ? channel : PUtil::GetChannelTitle(channel), "Work in progress", PUtil::GetLumiTitle(era));
        PUtil::DrawLegend(mainPad.get(), legend.get(), 2);

        for(std::string outdir: outdirs){
            canvas->SaveAs(StrUtil::Merge(outdir, "/limit_h", mh, ".pdf").c_str());
            canvas->SaveAs(StrUtil::Merge(outdir, "/limit_h", mh, ".png").c_str());    
        }

        mainPad->Clear();   
        legend->Clear();
    }

    mainPad->SetLogy(0);

    //Draw 2D limit plot 
    std::shared_ptr<TH2D> frameHist = RUtil::CloneSmart(expected2D->GetHistogram());

    frameHist->GetXaxis()->SetTitle("m(H^{#pm}) [GeV]");
    frameHist->GetXaxis()->SetLimits(*std::min_element(chargedMasses.begin(), chargedMasses.end()),
                                      *std::max_element(chargedMasses.begin(), chargedMasses.end()));  
    frameHist->GetYaxis()->SetLimits(*std::min_element(neutralMasses.begin(), neutralMasses.end()),
                                      *std::max_element(neutralMasses.begin(), neutralMasses.end()));  
    PUtil::SetHist(mainPad.get(), frameHist.get(), "m(h) [GeV]");
        
    frameHist->Draw();
    expected2D->Draw("SAME COL");
  //  theory2D->Draw("SAME L");

    PUtil::DrawHeader(mainPad.get(), channel == "Combined" ? channel : PUtil::GetChannelTitle(channel), "Work in progress", PUtil::GetLumiTitle(era));

    for(std::string outdir: outdirs){
        canvas->SaveAs(StrUtil::Merge(outdir, "/limit.pdf").c_str());
        canvas->SaveAs(StrUtil::Merge(outdir, "/limit.png").c_str());    
    }
}
