#include <ChargedHiggs/analysis/interface/plotter1D.h>

Plotter1D::Plotter1D() : Plotter(){}

Plotter1D::Plotter1D(std::string &histdir, std::vector<std::string> &xParameters) :
    Plotter(histdir, xParameters),
    background({}),
    signal({}),
    data({}),
    colors({
        {"DY+j", kRed + -7}, 
        {"TT+X", kYellow -6},             
        {"T+X", kGreen  + 2},             
        {"W+j", kCyan + 2},             
        {"QCD", kBlue -3},             
        {"VV+VVV", kViolet -3},
        {"TT", kOrange +2},
    })
 {}

void Plotter1D::ConfigureHists(std::vector<std::string> &processes){
    //Lambda function for sorting Histograms
    std::function<bool(TH1F*,TH1F*)> sortFunc = [](TH1F* hist1, TH1F* hist2){return hist1->Integral() < hist2->Integral();};

    for(std::string parameter: xParameters){
        //Define Vector for processes
        std::vector<TH1F*> bkgHists;
        std::vector<TH1F*> signalHists;

        for(std::string process: processes){
            //Get Histogram for parameter
            std::string filename = histdir + "/" + process + ".root";

            TFile* file = TFile::Open(filename.c_str());
            TH1F* hist = (TH1F*)file->Get(parameter.c_str());

            //Data configuration
            if(procDic[process] == DATA){
                hist->SetMarkerStyle(20);
                hist->SetName("data");

                data.push_back(hist);
            }

            //Signal configuration
            else if(procDic[process] == SIGNAL){
	            hist->SetLineColor(kBlack + signalHists.size());
                hist->SetLineWidth(4);

                std::vector<std::string> massStrings;
                std::string massString;
                std::istringstream massStream(process);
                while (std::getline(massStream, massString,  '_')){
                      massStrings.push_back(massString);
                }

                hist->SetName(("H^{#pm}_{" + massStrings[1] + "}+h_{" + massStrings[2] + "}").c_str());

                signalHists.push_back(hist);

            }

            //Background configuration
            else{
                hist->SetFillStyle(1001);
                hist->SetFillColor(colors.at(process));
                hist->SetLineColor(colors.at(process));
            
                hist->SetName(process.c_str());            

                bkgHists.push_back(hist);
            }
        }

        //Sort hist by integral                
        std::sort(bkgHists.begin(), bkgHists.end(), sortFunc);
        std::sort(signalHists.begin(), signalHists.end(), sortFunc);

        //Push back vector for each parameter to collection
        background.push_back(bkgHists);
        signal.push_back(signalHists);
    }
}

void Plotter1D::Draw(std::vector<std::string> &outdirs){

    for(unsigned int i = 0; i < xParameters.size(); i++){
        //Define canvas and pads
        TCanvas* canvas = new TCanvas((std::string("canvas") + std::to_string(i)).c_str(), (std::string("canvas") + std::to_string(i)).c_str(), 1000, 800);
        TPad* mainpad = new TPad((std::string("mainpad") + std::to_string(i)).c_str(), (std::string("mainpad") + std::to_string(i)).c_str(), 0., !data.empty() or !signal[i].empty()? 0.25: 0. , 0.95, 1.);
        TPad* legendpad = new TPad((std::string("legendpad") + std::to_string(i)).c_str(), (std::string("legendpad") + std::to_string(i)).c_str(), 0.87, !data.empty() or !signal[i].empty() ? 0.5: 0.3 , 1., 0.8);
        TPad* pullpad = NULL;

        if(!data.empty() or !signal.empty()){
            //Create pad for pull plot if data is not empty and resize canvas
            pullpad =  new TPad((std::string("pullpad") + std::to_string(i)).c_str(), (std::string("pullpad") + std::to_string(i)).c_str(), 0., 0.0 , 0.95, .25);
 
            canvas->SetWindowSize(1000,1000);  
            canvas->cd();
            pullpad->SetBottomMargin(0.2);
            pullpad->SetLeftMargin(0.15); 
            pullpad->Draw();
        }

        //TLegend
        TLegend* legend = new TLegend(0.0, 0.0, 1.0, 1.0);

        //Draw main pad
        mainpad->SetLeftMargin(0.15);
        mainpad->SetRightMargin(0.1);
        mainpad->SetBottomMargin(0.12);
        mainpad->Draw();
        mainpad->cd();
    
        //Stack and sum of backgrounds hist
        THStack* stack = new THStack();
        TH1F* sumbkg = (TH1F*)background[i][0]->Clone();
        sumbkg->Reset();
       

        for(TH1F* hist: background[i]){            
            std::string lentry = std::string(hist->GetName());
            legend->AddEntry(hist, lentry.c_str(), "F");

            //Fill sum of bkg hist and THStack
            sumbkg->Add(hist);
            stack->Add(hist);
        }
               
        //Configure and draw THStack
        stack->Draw("HIST");
        stack->GetXaxis()->SetTitleSize(0.05);
        stack->GetYaxis()->SetTitleSize(0.05);
        stack->GetXaxis()->SetTitleOffset(1.1);
        stack->GetXaxis()->SetTitle(sumbkg->GetXaxis()->GetTitle());
        stack->GetYaxis()->SetTitle("Events");

        stack->GetXaxis()->SetLabelSize(0.05);
        stack->GetYaxis()->SetLabelSize(0.05);

        //Maximum value for scaling axis
        float maximum = data.size() != 0 ? std::max(stack->GetMaximum(), data[i]->GetMaximum()) : stack->GetMaximum();
        stack->SetMaximum(maximum*1.25); 

        //Draw data
        if(!data.empty()){
            std::string lentry = std::string(data[i]->GetName());
            legend->AddEntry(data[i], lentry.c_str(), "P");
            data[i]->Draw("SAME EP");

        }

        //Draw signal
        if(!signal.empty()){
            for(TH1F* hist: signal[i]){
                std::string lentry = std::string(hist->GetName());
                legend->AddEntry(hist, lentry.c_str(), "L");
                hist->Draw("SAME HIST");
            }
        }

        //Draw legend
        canvas->cd();
        legendpad->Draw();
        legendpad->cd();

        legend->SetTextSize(0.2);
        legend->Draw();

        mainpad->cd();

        //Draw CMS/lumi info
        this->DrawHeader(true, "#mu + 4 jets", "Work in progress");
           
        //If you have data, draw Data/MC ratio
        if(!data.empty()){
            //Fill hist for pull plot
            TH1F* pull = (TH1F*)sumbkg->Clone();
            pull->Reset();

            for(int j = 1; j < sumbkg->GetNbinsX(); j++){
                float pullvalue = 0.;

                if(sumbkg->GetBinError(j) != 0){          
                   // pullvalue = (data[i]->GetBinContent(j) - sumbkg->GetBinContent(j))/sumbkg->GetBinError(j);
                    pullvalue = sumbkg->GetBinContent(j) != 0 ? data[i]->GetBinContent(j)/sumbkg->GetBinContent(j) : 1;
                }

                pull->SetBinContent(j, pullvalue);
            }
           
            pull->SetMarkerStyle(20);
            pull->SetTickLength(0.14, "X");
            pull->SetLabelSize(0.14, "XY");
            pull->SetTitleOffset(0.5, "Y");
            pull->SetTitleSize(0., "X");
            pull->SetTitleSize(0.12, "Y");
            pull->GetYaxis()->SetTitle("#frac{N_{data}}{N_{bkg}}");

            //Pull pad settings
		    pullpad->cd();
            pull->Draw("EP");  

            //Important, go back to mainpad
            mainpad->cd(); 
        }
 

        //If you have signal and not data, draw s/b 
        if(!signal[i].empty() and data.empty()){
            //Fill hist for pull plot
            TH1F* significance = (TH1F*)sumbkg->Clone();
            significance->Reset();

            for(int j = 1; j < sumbkg->GetNbinsX(); j++){
                float value = 0.;

                if(sumbkg->GetBinError(j) != 0){          
                    value = sumbkg->GetBinContent(j) != 0 ? signal[i][0]->GetBinContent(j)/TMath::Sqrt(sumbkg->GetBinContent(j)) : 0;
                }
    
                significance->SetBinContent(j, value);
            }

            significance->SetLineColor(kBlack);
            significance->SetLineWidth(4);
            significance->SetFillStyle(3353);
            significance->SetFillColor(kGray);
            significance->SetTickLength(0.14, "X");
            significance->SetLabelSize(0.14, "XY");
            significance->SetTitleOffset(0.5, "Y");
            significance->SetTitleSize(0., "X");
            significance->SetTitleSize(0.12, "Y");
            significance->GetYaxis()->SetTitle("#frac{N_{S}}{#sqrt{N_{B}}}");
           
            //Pull pad settings
		    pullpad->cd();
            significance->Draw("HIST");  

            //Important, go back to mainpad
            mainpad->cd(); 
        }

       
        
        //Save canvas in non-log and log scale
        for(std::string outdir: outdirs){
            canvas->SaveAs((outdir + "/" + xParameters[i] + ".pdf").c_str());
            canvas->SaveAs((outdir + "/" + xParameters[i] + ".png").c_str());
        }
        
        stack->SetMinimum(1e-1); 
        stack->SetMaximum(maximum*1e3); 
        mainpad->SetLogy(1);

        for(std::string outdir: outdirs){
            canvas->SaveAs((outdir + "/" + xParameters[i] + "_log.pdf").c_str());
            canvas->SaveAs((outdir + "/" + xParameters[i] + "_log.png").c_str());
        }

        if(!signal[i].empty()){
            //Draw shape plot
            canvas->SetWindowSize(1000,800); 
            mainpad->SetLogy(0);
            mainpad->Clear();
            mainpad->SetPad(0., 0. , 0.95, 1.);
            legend->Clear();

            float max = std::max(signal[i][0]->GetMaximum()/signal[i][0]->Integral(), sumbkg->GetMaximum()/sumbkg->Integral());

            //frame hist because scaling axis does not work with normalized draw 
            TH1F* frame = (TH1F*)sumbkg->Clone();
            frame->Reset();
            
            frame->SetMaximum(1.3*max); 
            frame->GetXaxis()->SetTitleSize(0.05);
            frame->GetYaxis()->SetTitleSize(0.05);
            frame->GetXaxis()->SetTitleOffset(1.1);
            frame->GetYaxis()->SetTitle("Events");
            frame->GetXaxis()->SetLabelSize(0.05);
            frame->GetYaxis()->SetLabelSize(0.05);
            frame->Draw();

            //Bkg hist fill style
            sumbkg->SetFillStyle(3353);
            sumbkg->SetFillColor(kRed);
            sumbkg->SetLineColor(kRed+1);
            sumbkg->SetLineWidth(4);

            legend->AddEntry(sumbkg, "Bkg", "F");
            sumbkg->DrawNormalized("HIST SAME");

            //Sig hist fill style
            signal[i][0]->SetFillStyle(3335);
            signal[i][0]->SetFillColor(kBlue);
            signal[i][0]->SetLineColor(kBlue+1);
            signal[i][0]->SetLineWidth(4);
               
            legend->AddEntry(signal[i][0], "Signal", "F");
            signal[i][0]->DrawNormalized("HIST SAME");

            this->DrawHeader(false, "#mu + 4 jets", "Work in progress");

            for(std::string outdir: outdirs){
                canvas->SaveAs((outdir + "/" + xParameters[i] + "_shape.pdf").c_str());
                canvas->SaveAs((outdir + "/" + xParameters[i] + "_shape.png").c_str());
            }

        }

        delete canvas;
        std::cout << "Plot created for: " << xParameters[i] << std::endl;
    }
}
