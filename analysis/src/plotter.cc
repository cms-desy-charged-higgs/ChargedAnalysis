#include <ChargedHiggs/analysis/interface/plotter.h>

Plotter::Plotter(){}

Plotter::Plotter(std::string &histdir, std::vector<std::string> &parameters) :
    histdir(histdir),
    parameters(parameters),
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

void Plotter::ConfigureHists(std::vector<std::string> &processes){
    //Lambda function for sorting Histograms
    std::function<bool(TH1F*,TH1F*)> sortFunc = [](TH1F* hist1, TH1F* hist2){return hist1->Integral() < hist2->Integral();};

    for(std::string parameter: parameters){
        //Define Vector for processes
        std::vector<TH1F*> bkgHists;
        std::vector<TH1F*> signalHists;

        for(std::string process: processes){
            //Get Histogram for parameter
            std::string filename = histdir + "/" + process + ".root";

            TFile* file = TFile::Open(filename.c_str());
            TH1F* hist = (TH1F*)file->Get(parameter.c_str());

            //Data configuration
            if(process.find("Single") != std::string::npos){
                hist->SetMarkerStyle(20);
                hist->SetName("data");

                data.push_back(hist);
            }

            //Signal configuration
            else if(process.find("L4B") != std::string::npos){
	            hist->SetLineColor(kBlack);
                hist->SetLineWidth(4);
                hist->SetName("H^{#pm}_{150}+h_{75}");

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

void Plotter::Draw(std::vector<std::string> &outdirs){
    //Style options
    TGaxis::SetMaxDigits(3);
    gStyle->SetLegendBorderSize(0);
    gStyle->SetFillStyle(0);
    gStyle->SetOptStat(0);
    gStyle->SetOptTitle(0);
    gROOT->SetBatch();
    gErrorIgnoreLevel = kWarning;

    //CMS Work in Progres and Lumi information
    TLatex* channel_title = new TLatex();
    channel_title->SetTextFont(42);
    channel_title->SetTextSize(0.045);

    TLatex* lumi = new TLatex();
    lumi->SetTextFont(42);
    lumi->SetTextSize(0.045);

    TLatex* cms = new TLatex();
    cms->SetTextFont(62);
    cms->SetTextSize(0.05);

    TLatex* work = new TLatex();
    work->SetTextFont(52);
    work->SetTextSize(0.045);


    for(unsigned int i = 0; i < parameters.size(); i++){
        //Define canvas and pads
        TCanvas* canvas = new TCanvas((std::string("canvas") + std::to_string(i)).c_str(), (std::string("canvas") + std::to_string(i)).c_str(), 1000, 800);
        TPad* mainpad = new TPad((std::string("mainpad") + std::to_string(i)).c_str(), (std::string("mainpad") + std::to_string(i)).c_str(), 0., !data.empty() ? 0.25: 0. , 1., 1.);
        TPad* pullpad = NULL;

        if(!data.empty()){
            //Create pad for pull plot if data is not empty and resize canvas
            pullpad =  new TPad((std::string("pullpad") + std::to_string(i)).c_str(), (std::string("pullpad") + std::to_string(i)).c_str(), 0., 0.0 , 1., .25);
 
            canvas->SetWindowSize(1000,1000);  
        }

        //TLegend
        TLegend* legend = new TLegend(0.65, 0.60, 1.0, 0.9);

        //Draw main pad
        mainpad->SetLeftMargin(0.15);
        mainpad->SetBottomMargin(0.12);
        mainpad->Draw();
        mainpad->cd();
    
        //Stack and sum of backgrounds hist
        THStack* stack = new THStack();
        TH1F* sumbkg = (TH1F*)background[i][0]->Clone();
        sumbkg->Reset();
       

        for(TH1F* hist: background[i]){            
            std::string lentry = std::string(hist->GetName()) + " [" + std::to_string((int)hist->Integral()) + "]";
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
            std::string lentry = std::string(data[i]->GetName()) + " [" + std::to_string((int)data[i]->Integral()) + "]";
            legend->AddEntry(data[i], lentry.c_str(), "P");
            data[i]->Draw("SAME EP");

        }

        //Draw signal
        if(!signal.empty()){
            for(TH1F* hist: signal[i]){
                std::string lentry = std::string(hist->GetName()) + " [" + std::to_string((int)hist->Integral()) + "]";
                legend->AddEntry(hist, lentry.c_str(), "L");
                hist->Scale(1./10.*maximum);
                hist->Draw("SAME HIST");
            }
        }

        //Draw legend and CMS/Lumi info
        legend->Draw("SAME");

        channel_title->DrawLatexNDC(0.17, 0.905, "#mu + <= 4 jets");
        cms->DrawLatexNDC(0.32, 0.905, "CMS");
        work->DrawLatexNDC(0.388, 0.905, "Work in progress");
        lumi->DrawLatexNDC(0.65, 0.905, "41.4 fb^{-1} (2017, 13 TeV)");
           
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
           
            //Pull pad settings
		    canvas->cd();
		    pullpad->Draw();
		    pullpad->cd();
            pullpad->SetBottomMargin(0.2);
            pullpad->SetLeftMargin(0.15); 

            pull->SetMarkerStyle(20);
            pull->SetTickLength(0.14, "X");
            pull->SetLabelSize(0.14, "XY");
            pull->GetYaxis()->SetTitle("#frac{data}{MC}");
            pull->SetTitleOffset(0.5, "Y");
            pull->SetTitleSize(0., "X");
            pull->SetTitleSize(0.12, "Y");
            pull->Draw("EP");  

            //Important, go back to mainpad
            mainpad->cd(); 
        }
 
         
        TGaxis::SetExponentOffset(-0.05, 0.005, "y");

        //Save canvas in non-log and log scale
        for(std::string outdir: outdirs){
            canvas->SaveAs((outdir + "/" + parameters[i] + ".pdf").c_str());
            canvas->SaveAs((outdir + "/" + parameters[i] + ".png").c_str());
        }
        
        stack->SetMinimum(1e-1); 
        stack->SetMaximum(maximum*1e3); 
        mainpad->SetLogy();

        for(std::string outdir: outdirs){
            canvas->SaveAs((outdir + "/" + parameters[i] + "_log.pdf").c_str());
            canvas->SaveAs((outdir + "/" + parameters[i] + "_log.png").c_str());
        }

        std::cout << "Plot created for: " << parameters[i] << std::endl;
    }
}
