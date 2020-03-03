#include <ChargedAnalysis/Analysis/include/plotter1D.h>

Plotter1D::Plotter1D() : Plotter(){}

Plotter1D::Plotter1D(std::string &histdir, std::string &channel, std::vector<std::string> &processes) :
    Plotter(histdir), 
    channel(channel),
    processes(processes),
    background({}),
    signal({}),
    data({})
 {}

void Plotter1D::ConfigureHists(){
    for(std::string process: processes){
        std::string fileName = histdir + "/" + process + "/" + process + ".root";
        TFile* file = TFile::Open(fileName.c_str());

        if(parameters.empty()){
            for(int i = 0; i < file->GetListOfKeys()->GetSize(); i++){
                parameters.push_back(file->GetListOfKeys()->At(i)->GetName());
            }
        }

        for(std::string& param: parameters){
            TH1F* hist = (TH1F*)file->Get(param.c_str());

            if(hist == NULL){
                throw std::runtime_error("Did not found histogram '" + param + "' in file '" + fileName + "'");
            }

            //Data configuration
            if(procDic[process] == DATA){
                hist->SetMarkerStyle(20);
                hist->SetName("data");

                data[param] = hist;
            }

            //Signal configuration
            else if(procDic[process] == SIGNAL){
                hist->SetLineWidth(1 + 3*signal[param].size());
                hist->SetLineColor(kBlack);

                std::vector<std::string> massStrings;
                std::string massString;
                std::istringstream massStream(process);
                while (std::getline(massStream, massString,  '_')){
                      massStrings.push_back(massString);
                }

                hist->SetName(("H^{#pm}_{" + massStrings[0].substr(5,7)
 + "}+h_{" + massStrings[1].substr(1,3) + "}").c_str());

                signal[param].push_back(hist);
            }

            //Background configuration
            else{
                hist->SetFillStyle(1001);
                hist->SetFillColor(colors.at(process));
                hist->SetLineColor(colors.at(process));
            
                hist->SetName(process.c_str());            

                background[param].push_back(hist);
            }
        }
    }
}

void Plotter1D::Draw(std::vector<std::string> &outdirs){
    //Lambda function for sorting Histograms
    std::function<bool(TH1F*,TH1F*)> sortFunc = [](TH1F* hist1, TH1F* hist2){return hist1->Integral() < hist2->Integral();};

    //Set Style
    Plotter::SetStyle();

    //Define canvas and pads
    TCanvas* canvas = new TCanvas("canvas",  "canvas", 1000, 800);
    TPad* mainpad = new TPad("mainpad", "mainpad", 0., 0. , 0.95, 1.);
    TPad* legendpad = new TPad("legendpad", "legendpad", 0.87, 0.3 , 1., 0.8);
    TPad* pullpad = new TPad("pullpad", "pullpad", 0., 0.0 , 0.95, .25);

    for(std::string& param: parameters){
        canvas->cd();

        if(!data.empty()){
            //Create pad for pull plot if data is not empty and resize canvas
            canvas->SetWindowSize(1000,1000);  
            mainpad->SetPad(0., 0.25 , 0.95, 1.);
            legendpad->SetPad(0.87, 0.5, 1., 0.8);

            canvas->cd();
            pullpad->SetBottomMargin(0.2);
            pullpad->SetLeftMargin(0.15); 
            pullpad->Draw();
        }

        //TLegend
        TLegend* legend = new TLegend(0.0, 0.0, 1.0, 1.0);

        //Stack and sum of backgrounds hist
        THStack* stack = new THStack();
        TH1F* sumbkg = (TH1F*)background[param][0]->Clone();
        sumbkg->Reset();

        //Draw main pad
        Plotter::SetPad(mainpad);
        mainpad->Draw();
        mainpad->cd();

        std::sort(background[param].begin(), background[param].end(), sortFunc);
    
        for(TH1F* hist: background[param]){            
            std::string lentry = std::string(hist->GetName());
            legend->AddEntry(hist, lentry.c_str(), "F");

            //Fill sum of bkg hist and THStack
            sumbkg->Add(hist);
            stack->Add(hist);
        }
               
        //Configure and draw THStack
        stack->Draw("HIST");
        Plotter::SetHist(stack->GetHistogram());
        stack->GetXaxis()->SetTitle(sumbkg->GetXaxis()->GetTitle());
        stack->GetYaxis()->SetTitle("Events");

        //Draw errorband
        sumbkg->SetFillStyle(3354);
        sumbkg->SetFillColorAlpha(kBlack, 0.8);
        sumbkg->SetMarkerColor(kBlack);
            
        sumbkg->SetName("Bkg. unc.");     
        sumbkg->Draw("SAME E2");    
        legend->AddEntry(sumbkg, "Bkg. unc.", "F");

        //Maximum value for scaling axis
        float maximum = data.size() != 0 ? std::max(stack->GetMaximum(), data[param]->GetMaximum()) : stack->GetMaximum();
        stack->SetMaximum(maximum*1.25); 

        //Draw data
        if(!data.empty()){
            std::string lentry = std::string(data[param]->GetName());
            legend->AddEntry(data[param], lentry.c_str(), "P");
            data[param]->Draw("SAME EP");

        }

        //Draw signal
        if(!signal.empty()){
            for(TH1F* hist: signal[param]){
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
        Plotter::DrawHeader(data.size() == 0 ? false : true, channelHeader[channel], "Work in progress");
           
        //If you have data, draw Data/MC ratio
        if(!data.empty()){
            //Fill hist for pull plot
            TH1F* pull = (TH1F*)sumbkg->Clone();
            pull->Reset();

            for(int j = 1; j <= sumbkg->GetNbinsX(); j++){
                float pullvalue = 0.;

                if(sumbkg->GetBinError(j) != 0){          
                   // pullvalue = (data[i]->GetBinContent(j) - sumbkg->GetBinContent(j))/sumbkg->GetBinError(j);
                    pullvalue = sumbkg->GetBinContent(j) != 0 ? data[param]->GetBinContent(j)/sumbkg->GetBinContent(j) : 1;
                }

                pull->SetBinContent(j, pullvalue);
            }
           
            pull->SetMinimum(0.5);
            pull->SetMaximum(1.5);
            pull->SetMarkerStyle(20);
            pull->SetTickLength(0.14, "X");
            pull->SetLabelSize(0.14, "XY");
            pull->SetTitleOffset(0.5, "Y");
            pull->SetTitleSize(0., "X");
            pull->SetTitleSize(0.12, "Y");
            pull->SetMarkerColor(kBlack);
            pull->GetYaxis()->SetTitle("#frac{N_{data}}{N_{bkg}}");

            //Error band
            TH1F* err =  (TH1F*)sumbkg->Clone();
            err->Divide(sumbkg);

            //Pull pad settings
		    pullpad->cd();
            pull->Draw("EP");  
            err->Draw("SAME E2");

            //Important, go back to mainpad
            mainpad->cd(); 
        }

        
        //Save canvas in non-log and log scale
        mainpad->SetLogy(0);

        for(std::string outdir: outdirs){
            canvas->SaveAs((outdir + "/" + param + ".pdf").c_str());
            canvas->SaveAs((outdir + "/" + param + ".png").c_str());
        }
        
        stack->SetMinimum(1e-1); 
        stack->SetMaximum(maximum*1e3); 
        mainpad->SetLogy(1);

        for(std::string outdir: outdirs){
            canvas->SaveAs((outdir + "/" + param + "_log.pdf").c_str());
            canvas->SaveAs((outdir + "/" + param + "_log.png").c_str());
        }

        if(!signal.empty()){
            for(TH1F* hist: signal[param]){
                //Clear pads
                for(TPad* pad: {legendpad, mainpad}){
                    pad->cd();
                    pad->Clear();
                }

                //Draw shape plot
                canvas->SetWindowSize(1000,800); 
                mainpad->SetLogy(0);
                mainpad->SetPad(0., 0. , 0.95, 1.);
                legendpad->SetPad(0.87, 0.3, 1., 0.8);

                float max = std::max(hist->GetMaximum()/hist->Integral(), sumbkg->GetMaximum()/sumbkg->Integral());

                //TLegend
                TLegend* legendSig = new TLegend(0.0, 0.0, 1.0, 1.0);
                legendSig->AddEntry(sumbkg, "Bkg", "F");
                legendSig->AddEntry(hist, "Signal", "F");

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

                sumbkg->DrawNormalized("HIST SAME");

                //Sig hist fill style
                hist->SetFillStyle(3335);
                hist->SetFillColor(kBlue);
                hist->SetLineColor(kBlue+1);
                hist->SetLineWidth(4);
                   
                hist->DrawNormalized("HIST SAME");

                Plotter::DrawHeader(false, channelHeader[channel], "Work in progress");

                legendpad->cd();
                legendSig->SetTextSize(0.2);
                legendSig->Draw();
               
                //Save canvas
                std::string massString = {hist->GetName()[9], hist->GetName()[10], hist->GetName()[11]};

                for(std::string outdir: outdirs){
                    canvas->SaveAs((outdir + "/" + param + "_" + massString + "_shape.pdf").c_str());
                    canvas->SaveAs((outdir + "/" + param + "_" + massString + "_shape.png").c_str());
                }
            }
        }

        std::cout << "Plot created for: " << param << std::endl;

        //Clear pads
        for(TPad* pad: {mainpad, pullpad, legendpad}){
            pad->cd();
            pad->Clear();
        }
    }
}
