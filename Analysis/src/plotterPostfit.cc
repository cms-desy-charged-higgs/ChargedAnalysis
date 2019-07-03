#include <ChargedHiggs/Analysis/interface/plotterPostfit.h>

PlotterPostfit::PlotterPostfit() : Plotter(){}

PlotterPostfit::PlotterPostfit(std::string &limitDir, int &mass, std::vector<std::string> &channel) : 
    Plotter(),
    limitDir(limitDir),
    mass(mass),
    channel(channel){

    chanToDir = {{"mu4j", "Muon4J"}, {"mu2j1f", "Muon2J1F"}, {"mu2f", "Muon2F"}, {"e4j", "Ele4J"}, {"e2j1f", "Ele2J1F"}, {"e2f", "Ele2F"}};

    for(std::string chan: channel){
        backgrounds[chan] = std::vector<TH1F*>();
    }

}

void PlotterPostfit::ConfigureHists(std::vector<std::string> &processes){
    //Lambda function for sorting Histograms
    std::function<bool(TH1F*,TH1F*)> sortFunc = [](TH1F* hist1, TH1F* hist2){return hist1->Integral() < hist2->Integral();};

    for(std::string chan: channel){
        //Open file with Postfit shapes
        TFile* file = TFile::Open((limitDir + "/" + std::to_string(mass) + "/fitshapes.root").c_str());
        TDirectory* dir = (TDirectory*)file->Get((chan + "_postfit").c_str());
        TList* keys = dir->GetListOfKeys();

        //Configure all histograms
        for(int i=0; i < keys->GetSize(); i++){
            std::string processName = std::string(keys->At(i)->GetName());

            TH1F* hist = (TH1F*)dir->Get(processName.c_str());

            if(processName == "HPlus"){
                hist->SetLineWidth(2);
                hist->SetLineColor(kBlack);
                hist->SetName(("H^{#pm}_{" + std::to_string(mass) + "}+h_{100}").c_str());

                signals[chan] = hist;
            }
       
            else if(processName == "TotalBkg"){
                hist->SetFillStyle(3354);
                hist->SetFillColorAlpha(kBlack, 0.8);
                hist->SetMarkerColor(kBlack);
                hist->SetName("Bkg. unc."); 

                errorBand[chan] = hist;

                max = max < hist->GetMaximum() ? hist->GetMaximum() : max;
            }
    
            else if(processName != "data_obs" and processName != "TotalProcs" and processName != "TotalSig"){
                hist->SetFillStyle(1001);
                hist->SetFillColor(colors.at(processName));
                hist->SetLineColor(colors.at(processName));
                hist->SetName(processName.c_str());            

                backgrounds[chan].push_back(hist);
            }       
        }

        std::sort(backgrounds[chan].begin(), backgrounds[chan].end(), sortFunc);
    }
}

void PlotterPostfit::Draw(std::vector<std::string> &outdirs){
    //Set Style
    this->SetStyle();

    //Define canvas and pads
    TCanvas* canvas = new TCanvas("canvas",  "canvas", 1400, 800);
    TPad* legendpad = new TPad("legendpad", "legendpad", 0.91, 0.25, 1., 0.9);
    TLegend* legend = new TLegend(0.15, 0.2, 0.75, 0.8);

    for(unsigned int i=0; i < channel.size(); i++){    
        canvas->cd();

        float padStart = i == 0 ? 0.05 - 0.3*(0.85/6.) + 0.85/6.*i: 0.05 + 0.85/6.*i;
        float padEnd= 0.05 + 0.85/6.*(i+1);
        
        //Draw Tpad
        TPad* pad = new TPad(("pad_" + channel[i]).c_str(), ("pad_" + channel[i]).c_str(), padStart, .2 , padEnd, 1.);
        TPad* pullpad = new TPad("pullpad", "pullpad", padStart, 0.0 , padEnd, .2);


        pullpad->SetTopMargin(0.05);
        pullpad->SetBottomMargin(0.15);

        for(TPad* p: {pad, pullpad}){
            p->SetLeftMargin(0.);
            p->SetRightMargin(0.);

            if(i==0){
                p->SetLeftMargin(0.3);
            }
        }

        pad->Draw();
        pad->cd();
        pad->SetLogy(1);

        //Fill THStack;
        THStack* stack = new THStack(("channel_" + channel[i]).c_str(), ("stack_" + channel[i]).c_str());

        for(TH1F* hist: backgrounds[channel[i]]){      
            if(i==0){      
                legend->AddEntry(hist, hist->GetName(), "F");
            }

            //Fill sum of bkg hist and THStack
            stack->Add(hist);
        }

        stack->Draw("HIST");
        stack->SetMinimum(1e-1); 
        stack->SetMaximum(max*2); 
        stack->GetXaxis()->SetNdivisions(5);
        stack->GetXaxis()->SetTickLength(0.01);

        if(i==0){
            stack->GetYaxis()->SetLabelSize(0.09);
            stack->GetXaxis()->SetLabelSize(0.09);
            stack->GetXaxis()->SetLabelOffset(-0.03);
            stack->GetYaxis()->SetTitle("Events");
            stack->GetYaxis()->SetTitleSize(0.1);
        }

        else if(i!=0){
            stack->GetXaxis()->SetLabelSize(0.12);
            stack->GetXaxis()->SetLabelOffset(-0.055);
            stack->GetYaxis()->SetLabelSize(0);
            stack->GetYaxis()->SetAxisColor(0,0);
        }

        if(i+1==channel.size()){
            stack->GetXaxis()->SetTitleSize(0.12);
            stack->GetXaxis()->SetTitle(signals[channel[i]]->GetXaxis()->GetTitle());
            stack->GetXaxis()->SetTitleOffset(0.35);
        }

        errorBand[channel[i]]->Draw("SAME E2"); 
        signals[channel[i]]->Draw("HIST SAME");

        if(i==0){      
            legend->AddEntry(signals[channel[i]], signals[channel[i]]->GetName(), "L");
            legend->AddEntry(errorBand[channel[i]], errorBand[channel[i]]->GetName(), "F");
        }

        TLatex* chanHeader = new TLatex();
        chanHeader->SetTextSize(i==0? 0.089 : 0.11);
        chanHeader->DrawLatexNDC(i ==0? 0.33 : 0.05, 0.87, channelHeader[channel[i]].c_str());

        canvas->cd();
        pullpad->Draw();
        pullpad->cd();

        TH1F* pullErrorband = (TH1F*)errorBand[channel[i]]->Clone();
        pullErrorband->Divide(errorBand[channel[i]]);

        pullErrorband->GetXaxis()->SetNdivisions(5);
        pullErrorband->GetYaxis()->SetNdivisions(5);
        pullErrorband->GetYaxis()->SetLabelSize(0.15);
        pullErrorband->GetXaxis()->SetLabelSize(0.15);
        pullErrorband->GetXaxis()->SetTitle("Pull");
        pullErrorband->SetMinimum(0.5);
        pullErrorband->SetMaximum(1.5);
        pullErrorband->SetTitleSize(0., "X");

        if(i==0){
            pullErrorband->GetYaxis()->SetTitle("Pull");
            pullErrorband->GetYaxis()->SetTitleSize(0.17);
            pullErrorband->GetYaxis()->SetTitleOffset(0.8);
        }

        else if(i!=0){
            pullErrorband->GetYaxis()->SetLabelSize(0);
            pullErrorband->GetYaxis()->SetAxisColor(0,0);
        }

        pullErrorband->Draw("E2");
    }

    canvas->cd();

    TLatex* cms = new TLatex();
    cms->SetTextFont(62);
    cms->SetTextSize(0.04);
    cms->DrawLatexNDC(0.2, 0.925, "CMS");

    TLatex* work = new TLatex();
    work->SetTextFont(52);
    work->SetTextSize(0.035);
    work->DrawLatexNDC(0.25, 0.925, "Work in Progress");

    TLatex* lumi = new TLatex();
    lumi->SetTextFont(42);
    lumi->SetTextSize(0.035);
    lumi->DrawLatexNDC(0.72, 0.925, "41.4 fb^{-1} (2017, 13 TeV)");

    legendpad->Draw();
    legendpad->cd();
    legend->SetTextSize(0.15);
    legend->Draw();
    
    for(std::string outdir: outdirs){
        canvas->SaveAs((outdir + "/postfit_" + std::to_string(mass) + ".pdf").c_str());
        canvas->SaveAs((outdir + "/postfit_" + std::to_string(mass) + ".png").c_str());
    }

    std::cout << "Plotfit plot created for: " << mass << std::endl;
}
