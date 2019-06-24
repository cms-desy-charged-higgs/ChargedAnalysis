#include <ChargedHiggs/analysis/interface/plotterCut.h>

PlotterCut::PlotterCut() : Plotter(){}

PlotterCut::PlotterCut(std::string &histdir, std::vector<std::string> &xParameters, std::string &channel) : Plotter(histdir, xParameters, channel) {}


Double_t PlotterCut::GetAsimov(const Double_t &s, const Double_t &b, const Double_t &sigB, const Double_t &sigS){
    return std::sqrt(2*((s+b)*std::log(1.+s/b) - s));
}

void PlotterCut::ConfigureHists(std::vector<std::string> &processes){
    std::function<bool(TH1F*,TH1F*)> sortFunc = [](TH1F* hist1, TH1F* hist2){return hist1->Integral() > hist2->Integral();};

    for(std::string parameter: xParameters){
        //Define Vector for processes
        std::vector<TH1F*> cutHist;
        std::vector<TH1F*> NsigHist;
        TH1F* NbkgHist = NULL;
        
        std::vector<TH1F*> sigHists;
        TH1F* bkgSum = NULL;

        for(std::string process: processes){
            //Get Histogram for parameter
            std::string filename = histdir + "/" + process + ".root";

            TFile* file = TFile::Open(filename.c_str());
            TH1F* hist = (TH1F*)file->Get(parameter.c_str());

            //Signal configuration
            if(procDic[process] == SIGNAL){
                std::vector<std::string> massStrings;
                std::string massString;
                std::istringstream massStream(process);
                while (std::getline(massStream, massString,  '_')){
                      massStrings.push_back(massString);
                }

                hist->SetName(("H^{#pm}_{" + massStrings[1] + "}+h_{" + massStrings[2] + "}").c_str());

                sigHists.push_back(hist);
            }

            //Background configuration
            else{
                if(bkgSum == NULL){
                    bkgSum = (TH1F*)hist->Clone();
                }

                else{
                    bkgSum->Add(hist);
                }
            }
        }

        for(TH1F* signal: sigHists){
            TH1F* cut = (TH1F*)signal->Clone();
            cut->Reset();
            TH1F* Nsig = (TH1F*)signal->Clone();
            Nsig->Reset();

            NbkgHist = (TH1F*)bkgSum->Clone();
            NbkgHist->Reset();

            Double_t errB, errS, intS, intB;

            for(int i = 0; i < signal->GetNbinsX() + 1; i++){
                intS = signal->IntegralAndError(i, signal->GetNbinsX()+1, errS);
                intB = bkgSum->IntegralAndError(i, bkgSum->GetNbinsX()+1, errB);

                Double_t asimov = GetAsimov(intS, intB, errB, errS);

                cut->SetBinContent(i, intB != 0 and errB != 0 ? asimov : 0);
                Nsig->SetBinContent(i, intS);
                NbkgHist->SetBinContent(i, intB);
            }

            Nsig->SetMarkerStyle(20 + cutHist.size());
            Nsig->SetMarkerColor(1 + cutHist.size());

            cut->SetMarkerStyle(20 + cutHist.size());
            cut->SetMarkerColor(1 + cutHist.size());

            cutHist.push_back(cut);
            NsigHist.push_back(Nsig);
        }

        NbkgHist->SetMarkerStyle(20 + cutHist.size());
        NbkgHist->SetMarkerColor(1 + cutHist.size());

        std::sort(cutHist.begin(), cutHist.end(), sortFunc);
        std::sort(NsigHist.begin(), NsigHist.end(), sortFunc);

        cutHists.push_back(cutHist);
        NsigHists.push_back(NsigHist);
        NbkgHists.push_back(NbkgHist);
    }
}

void PlotterCut::Draw(std::vector<std::string> &outdirs){
    //Set style
    this->SetStyle();

    //Define canvas and pads
    TCanvas* canvas = new TCanvas("canvas",  "canvas", 1000, 800);
    TPad* mainpad = new TPad("mainpad", "mainpad", 0., 0. , 0.95, 1.);
    TPad* legendpad = new TPad("legendpad", "legendpad", 0.87, 0.3 , 1., 0.8);

    std::vector<std::vector<std::vector<TH1F*>>> histograms = {cutHists, NsigHists};
    std::vector<std::string> yTitles = {"Asimov significance", "Num. events after cut"};
    std::vector<std::string> fileEnd = {"cut", "cutInt"};

    for(unsigned int i = 0; i < xParameters.size(); i++){
        for(unsigned j = 0; j < histograms.size(); j++){
            canvas->cd();

            //TLegend
            TLegend* legend = new TLegend(0.0, 0.0, 1.0, 1.0);

            //Draw main pad
            this->SetPad(mainpad);
            mainpad->Draw();
            mainpad->cd();
                   
            //Configure hist
            //Maximum value for scaling axis

            histograms[j][i][0]->Draw("P");
            histograms[j][i][0]->GetYaxis()->SetTitle(yTitles[j].c_str());
            histograms[j][i][0]->SetMaximum(j == 0 ? histograms[j][i][0]->GetMaximum()*1.25 : NbkgHists[i]->GetMaximum()*1e2);
            this->SetHist(histograms[j][i][0]);

            std::string lentry = std::string(histograms[j][i][0]->GetName());
            legend->AddEntry(histograms[j][i][0], lentry.c_str(), "P");

            for(unsigned int k = 1; k < histograms[j][i].size(); k++){
                std::string lentry = std::string(histograms[j][i][k]->GetName());
                legend->AddEntry(histograms[j][i][k], lentry.c_str(), "P");
                histograms[j][i][k]->Draw("P SAME");
            }        

            if(j==1){
                histograms[j][i][0]->SetMinimum(1e-1);
                mainpad->SetLogy(1);
                std::string lentry = std::string("Bkg");
                legend->AddEntry(NbkgHists[i], lentry.c_str(), "P");

                NbkgHists[i]->Draw("P SAME");
            }

            //Draw legend
            canvas->cd();
            legendpad->Draw();
            legendpad->cd();

            legend->SetTextSize(0.2);
            legend->Draw();

            mainpad->cd();

            //Draw CMS/lumi info
            this->DrawHeader(false, channelHeader[channel], "Work in progress");

            for(std::string outdir: outdirs){
                canvas->SaveAs((outdir + "/" + xParameters[i] + "_" + fileEnd[j] + ".pdf").c_str());
                canvas->SaveAs((outdir + "/" + xParameters[i] + "_" + fileEnd[j] + ".png").c_str());
            }

            //Clear pads
            for(TPad* pad: {mainpad, legendpad}){
                pad->cd();
                pad->Clear();
            }
        }

        std::cout << "Plot created for: " << xParameters[i] << std::endl;

    }
}
