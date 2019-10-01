#include <ChargedAnalysis/Analysis/include/plottertriggeff.h>

PlotterTriggEff::PlotterTriggEff() : Plotter(){}

PlotterTriggEff::PlotterTriggEff(std::string &histdir, std::string &total, std::vector<std::string> &passed,std::vector<std::string> &yParam, std::string &channel) : 
        Plotter(histdir, passed, channel),
        total(total),
        passed(passed),
        yParam(yParam) {
            ptNames = {{"ele+4j", "e_pt"}, {"mu+4j", "mu_pt"}};
            phiNames = {{"ele+4j", "e_phi"}, {"mu+4j", "mu_phi"}};
            etaNames = {{"ele+4j", "e_eta"}, {"mu+4j", "mu_eta"}};
        }


void PlotterTriggEff::ConfigureHists(std::vector<std::string> &processes){
    bkgEfficiencies = std::vector<std::vector<TGraphAsymmErrors*>>(yParam.size(), std::vector<TGraphAsymmErrors*>(passed.size(), NULL));

    dataEfficiencies = std::vector<std::vector<TGraphAsymmErrors*>>(yParam.size(), std::vector<TGraphAsymmErrors*>(passed.size(), NULL));

    //Save 2D hist for trigger information
    std::vector<TH2F*> bkgTotal(yParam.size(), NULL);
    std::vector<std::vector<TH2F*>> bkgPassed(yParam.size(), std::vector<TH2F*>(passed.size(), NULL));

    std::vector<TH2F*> dataTotal;

    //Loop over all processes
    for(std::string process: processes){
        std::string filename = histdir + "/" + process + ".root";

        TFile* file = TFile::Open(filename.c_str());

        for(unsigned int i = 0; i < yParam.size(); i++){
            if(procDic[process] == BKG){
                if(bkgTotal[i] == NULL){
                    bkgTotal[i] = (TH2F*)file->Get((total + "_VS_" + yParam[i]).c_str());
                }

                else{
                    bkgTotal[i]->Add((TH2F*)file->Get((total + "_VS_" + yParam[i]).c_str()));
                }

                for(unsigned int j = 0; j < passed.size(); j++){
                    TH2F* pass = (TH2F*)file->Get((passed[j] + "_VS_" + yParam[i]).c_str());

                    //Merge bkg hist to another
                    if(bkgPassed[i][j] == NULL){
                        bkgPassed[i][j] = pass;
                    }

                    //Push back hist empty
                    else{
                         bkgPassed[i][j]->Add(pass);
                    }
                }
            }

            else if(procDic[process] == DATA){
                dataTotal.push_back((TH2F*)file->Get((total + "_VS_" + yParam[i]).c_str()));

                for(unsigned int j = 0; j < passed.size(); j++){
                    TH2F* dataPassed = (TH2F*)file->Get((passed[j] + "_VS_" + yParam[i]).c_str());

                    TH1F* pass = (TH1F*)dataPassed->ProjectionY()->Clone();
                    pass->Reset();
                    pass->Sumw2(kFALSE);

                    TH1F* total = (TH1F*)dataPassed->ProjectionY()->Clone();
                    total->Reset();
                    total->Sumw2(kFALSE);


                    for(int k = 1; k <= dataPassed->GetYaxis()->GetNbins(); k++){
                        pass->SetBinContent(k, dataPassed->GetBinContent(2, k));
                        total->SetBinContent(k,dataTotal[i]->GetBinContent(2, k));
                    }
        
                    TGraphAsymmErrors* dataEff = new TGraphAsymmErrors();
                    dataEff->Divide(pass, total, "cp");
                    dataEff->SetMarkerStyle(20);

                    dataEfficiencies[i][j] = dataEff;
                }
            }
        }
    }

    for(unsigned int i = 0; i < yParam.size(); i++){
        for(unsigned int j = 0; j < bkgPassed[i].size(); j++){
            TH1F* passed = (TH1F*)bkgPassed[i][j]->ProjectionY()->Clone();
            passed->Reset();
            passed->Sumw2(kFALSE);

            TH1F* total = (TH1F*)bkgPassed[i][j]->ProjectionY()->Clone();
            total->Reset();
            total->Sumw2(kFALSE);

            for(int k = 1; k <= bkgPassed[i][j]->GetYaxis()->GetNbins(); k++){
                passed->SetBinContent(k, bkgPassed[i][j]->GetBinContent(2, k));
                total->SetBinContent(k,bkgTotal[i]->GetBinContent(2, k));
            }

            TGraphAsymmErrors* bkgEff = new TGraphAsymmErrors();
            bkgEff->Divide(passed, total, "cp");

            bkgEff->SetMarkerStyle(21);
            bkgEff->SetMarkerColor(kBlue);
            bkgEff->GetHistogram()->GetXaxis()->SetTitle(passed->GetXaxis()->GetTitle());
            bkgEff->GetHistogram()->GetXaxis()->SetTitleSize(0.05);
            bkgEff->GetHistogram()->GetYaxis()->SetTitleSize(0.05);
            bkgEff->GetHistogram()->GetXaxis()->SetTitleOffset(1.1);
            bkgEff->GetHistogram()->GetYaxis()->SetTitle("Efficiency");
            bkgEff->GetHistogram()->SetMaximum(1.); 

            bkgEfficiencies[i][j] = bkgEff;
        }
    }
}


void PlotterTriggEff::Draw(std::vector<std::string> &outdirs){
    TCanvas* canvas = new TCanvas("canvas", "canvas", 1000, 800);
    TPad* mainpad = new TPad("mainpad", "mainpad", 0., 0. , 0.95, 1.);
    TPad* legendpad = new TPad("legendpad", "legendpad", 0.87, 0.3 , 1., 0.8);

    mainpad->SetLeftMargin(0.15);
    mainpad->SetRightMargin(0.1);
    mainpad->SetBottomMargin(0.12);
    mainpad->Draw();
    legendpad->Draw();

    canvas->cd();

    this->DrawHeader(false, "e inclusive", "Work in progress");

    for(unsigned int i = 0; i < yParam.size(); i++){
        for(unsigned int j = 0; j < passed.size(); j++){
            mainpad->Clear();  
            legendpad->Clear();

            legendpad->cd();

            TLegend* legend = new TLegend(0.0, 0.0, 1.0, 1.0);
            legend->Draw();

            mainpad->cd();

            legend->AddEntry(bkgEfficiencies[i][j], "MC", "P");
            legend->AddEntry(dataEfficiencies[i][j], "data", "P");

            bkgEfficiencies[i][j]->Draw("AP"); 
            dataEfficiencies[i][j]->Draw("SAME P"); 

            for(std::string outdir: outdirs){
                canvas->SaveAs((outdir + "/" + passed[j] + "_" + yParam[i] + "_Eff.pdf").c_str());
                canvas->SaveAs((outdir + "/" + passed[j] + "_" + yParam[i] + "_Eff.png").c_str());
            }
        }
    }
}
