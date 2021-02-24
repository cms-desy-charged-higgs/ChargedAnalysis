#include <ChargedAnalysis/Analysis/include/plotterLimit.h>

PlotterLimit::PlotterLimit() : Plotter(){}

PlotterLimit::PlotterLimit(const std::vector<std::string> limitFiles, const std::vector<int>& chargedMasses, const std::vector<int>& neutralMasses, const std::string& channel, const std::string& era, const std::vector<float>& xSecs) : 
    Plotter(),
    limitFiles(limitFiles),
    chargedMasses(chargedMasses),
    neutralMasses(neutralMasses),
    channel(channel),
    era(era),
    xSecs(xSecs){}

void PlotterLimit::ConfigureHists(){
    expected = std::make_shared<TH2D>(("Ex" + channel).c_str(), ("Ex" + channel).c_str(), chargedMasses.size(), chargedMasses[0] - 50, chargedMasses.back() + 50, neutralMasses.size() + 1, neutralMasses[0] - 5, neutralMasses.back() + 5);

    theory = std::make_shared<TH2D>(("Theo" + channel).c_str(), ("Theo" + channel).c_str(), chargedMasses.size(), chargedMasses[0] - 50, chargedMasses.back() + 50, neutralMasses.size() + 1, neutralMasses[0] - 5, neutralMasses.back() + 5);

    for(unsigned int i=0; i < chargedMasses.size(); ++i){
        for(unsigned int j=0; j < neutralMasses.size(); ++j){
            int mHC = chargedMasses[i], mh = neutralMasses[j];
            int idx = MUtil::RowMajIdx({chargedMasses.size(), neutralMasses.size()}, {i, j});

            std::shared_ptr<TFile> limitFile = RUtil::Open(limitFiles.at(idx));
            std::shared_ptr<TTree> limitTree = RUtil::GetSmart<TTree>(limitFile.get(), "limit");

            //Set branch for reading limits
            std::vector<double> limitValues;
            TLeaf* limitValue = RUtil::Get<TLeaf>(limitTree.get(), "limit");       

            //Values
            for(int k=0; k < 5; k++){
                limitValues.push_back(RUtil::GetEntry<double>(limitValue, k) * xSecs[idx]);
            }

            expected->SetBinContent(i+1, j+1, limitValues[2]);    
            theory->SetBinContent(i+1, j+1, xSecs[idx]);

            if(i == 0 and j == 0){
                sigmaOne = std::make_shared<TGraphAsymmErrors>();
                sigmaTwo = std::make_shared<TGraphAsymmErrors>();

                sigmaOne->SetFillColor(kGreen);
                sigmaOne->SetFillStyle(1001);  

                sigmaTwo->SetFillColor(kYellow);
                sigmaTwo->SetFillStyle(1001);
            }

            sigmaOne->SetPoint(i, mHC, limitValues[2]);
            sigmaOne->SetPointEYlow(i, limitValues[2] - limitValues[1]);
            sigmaOne->SetPointEYhigh(i, limitValues[3] - limitValues[1]);  

            sigmaTwo->SetPoint(i, mHC, limitValues[2]);
            sigmaTwo->SetPointEYlow(i, limitValues[2] - limitValues[0]);
            sigmaTwo->SetPointEYhigh(i, limitValues[4] - limitValues[1]);
        }
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

        //Expected/theory hist configuration
        std::shared_ptr<TH1D> exHist(expected->ProjectionX("ex", j+1, j+2));
        exHist->SetLineWidth(3);
        exHist->SetLineColor(kBlack);

        std::shared_ptr<TH1D> theoHist(theory->ProjectionX("theo", j+1, j+2));
        theoHist->SetLineWidth(3);
        theoHist->SetLineStyle(2);
        theoHist->SetLineColor(kBlack);

        //Frame histo
        std::shared_ptr<TH1D> frameHist = RUtil::CloneSmart<TH1D>(exHist.get());
        frameHist->Reset();
        PUtil::SetHist(mainPad.get(), frameHist.get(), "95% CL Limit on #sigma(pp #rightarrow H^{#pm}h #rightarrow lb#bar{b}b#bar{b}) [pb]");
  
        float min = std::min(theoHist->GetMinimum(), sigmaTwo->GetHistogram()->GetMinimum());

        frameHist->GetXaxis()->SetLimits(chargedMasses[0], chargedMasses.back());
        frameHist->SetMinimum(min*1e-1);
        frameHist->GetXaxis()->SetTitle("m(H^{#pm}) [GeV]");

        //Add legend information            
        std::shared_ptr<TLegend> legend = std::make_shared<TLegend>(0.0, 0.0, 1.0, 1.0);

        legend->AddEntry(exHist.get(), "Expected", "L");
        legend->AddEntry(theoHist.get(), "Theo. prediction", "L");
        legend->AddEntry(sigmaOne.get(), "68% expected", "F");
        legend->AddEntry(sigmaTwo.get(), "95% expected", "F");

        //Draw everything and save
        frameHist->Draw();
        sigmaTwo->Draw("SAME 3");
        sigmaOne->Draw("SAME 3");
        exHist->Draw("C SAME");
        theoHist->Draw("C SAME");
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
    expected->GetXaxis()->SetTitle("m(H^{#pm}) [GeV]");
    expected->GetXaxis()->SetLimits(chargedMasses[0], chargedMasses.back());  
    expected->GetYaxis()->SetLimits(neutralMasses[0], neutralMasses.back() + 10);  
    PUtil::SetHist(mainPad.get(), expected.get(), "m(h) [GeV]");
        
    expected->Draw("COLZ");
    PUtil::DrawHeader(mainPad.get(), channel == "Combined" ? channel : PUtil::GetChannelTitle(channel), "Work in progress", PUtil::GetLumiTitle(era));

    for(std::string outdir: outdirs){
        canvas->SaveAs(StrUtil::Merge(outdir, "/limit.pdf").c_str());
        canvas->SaveAs(StrUtil::Merge(outdir, "/limit.png").c_str());    
    }
}
