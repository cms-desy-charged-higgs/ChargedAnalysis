#include <ChargedHiggs/analysis/interface/plotter.h>

Plotter::Plotter(){}

Plotter::Plotter(std::string &histdir) :
    histdir(histdir),
    background({}),
    data({}), 
    colors({
        {"DY+j", kRed + -7}, 
        {"TT+X", kYellow -6},             
        {"T+X", kGreen  + 2},             
        {"W+j", kCyan + 2},             
        {"QCD", kBlue -3},             
        {"VV+VVV", kViolet -3},
        {"TT", kOrange +2},
    }),
    xLabels({
        {"W_pt", "p_{T}(W) [GeV]"},
        {"W_phi", "#phi(W) [rad]"},
        {"W_mT", "m_{T}(W) [GeV]"},
        {"e_pt", "p_{T}(e) [GeV]"},
        {"nJet", "Number of Jets"},
        {"j1_pt", "p_{T}(j_{1}) [GeV]"},
        {"j2_pt", "p_{T}(j_{2}) [GeV]"},
        {"HT", "H_{T} [GeV]"},
        {"met", "E_{T}^{miss} [GeV]"},
    })
 {}

void Plotter::ConfigureHists(std::vector<std::string> &parameters, std::vector<std::string> &processes){
    for(std::string process: processes){
        std::string filename = histdir + "/" + process + ".root";

        TFile* file = TFile::Open(filename.c_str());

        for(std::string parameter: parameters){
            TH1F* hist = 0;

            file->GetObject(parameter.c_str(), hist);
            hist->SetName(process.c_str());

            if(process != "data"){
                hist->SetFillStyle(1001);
                hist->SetFillColor(colors.at(process));
                hist->SetLineColor(colors.at(process));
            
                if(!background.count(parameter)){
                    background[parameter] = {hist};
                }

                else{
                    background.at(parameter).push_back(hist);
                }
            }
    
            else{
                hist->SetMarkerStyle(20);
                data[parameter] = hist;
            }

        }
    }
}

void Plotter::Draw(std::vector<std::string> &outdirs){
    gStyle->SetLegendBorderSize(0);
    gStyle->SetFillStyle(0);
    gStyle->SetOptStat(0);
    gStyle->SetOptTitle(0);
    gROOT->SetBatch();
    gErrorIgnoreLevel = kWarning;

    TLatex* channel_title = new TLatex();
    channel_title->SetTextFont(42);
    channel_title->SetTextSize(0.035);

    TLatex* lumi = new TLatex();
    lumi->SetTextFont(42);
    lumi->SetTextSize(0.035);

    TLatex* cms = new TLatex();
    cms->SetTextFont(62);
    cms->SetTextSize(0.032);

    TLatex* work = new TLatex();
    work->SetTextFont(52);
    work->SetTextSize(0.035);

    std::function<bool(TH1F*,TH1F*)> sortFunc = [](TH1F* hist1, TH1F* hist2){return hist1->Integral() < hist2->Integral();};

    for(std::pair<std::string, std::vector<TH1F*>> hists: background){
        TCanvas* canvas = new TCanvas((std::string("canvas") + hists.first).c_str(), (std::string("canvas") + hists.first).c_str(), 1000, 800);

        TPad* mainpad = new TPad((std::string("mainpad") + hists.first).c_str(), (std::string("mainpad") + hists.first).c_str(), 0., 0.20 , 1., 1.);
        TPad* pullpad = new TPad((std::string("pullpad") + hists.first).c_str(), (std::string("pullpad") + hists.first).c_str(), 0., 0.0 , 1., .20);

        
        if(!data.empty()){
            canvas->SetWindowSize(1000,1000);  

            mainpad->SetLeftMargin(0.15);
            mainpad->Draw();
            mainpad->cd();
        }


        TH1F* sumbkg = (TH1F*)hists.second[0]->Clone();
        sumbkg->Reset();
        THStack* stack = new THStack();
        TLegend* legend = new TLegend(0.65, 0.65, 0.9, 0.9);
       
        std::sort(hists.second.begin(), hists.second.end(), sortFunc);

        for(TH1F* hist: hists.second){
            sumbkg->Add(hist);
            
            std::string lentry = std::string(hist->GetName()) + " [" + std::to_string((int)hist->Integral()) + "]";
            legend->AddEntry(hist, lentry.c_str(), "F");
            stack->Add(hist);
        }
                
        
        stack->Draw("HIST");
        stack->GetXaxis()->SetTitle(xLabels[hists.first].c_str());
        stack->GetYaxis()->SetTitle("Events");

        if(!data.empty()){
            std::string lentry = std::string(data[hists.first]->GetName()) + " [" + std::to_string((int)data[hists.first]->Integral()) + "]";
            legend->AddEntry(data[hists.first], lentry.c_str(), "P");
            data[hists.first]->Draw("SAME EP");

        }

        legend->Draw("SAME");

        //channel_title->DrawLatexNDC(0.17, 0.905, "e4bjets");
        cms->DrawLatexNDC(0.25, 0.905, "CMS");
        work->DrawLatexNDC(0.318, 0.905, "Work in progress");
        lumi->DrawLatexNDC(0.605, 0.905, "41.4 fb^{-1} (2017, 13 TeV)");

              

        if(!data.empty()){
            TH1F* pull = (TH1F*)hists.second[0]->Clone();
            pull->Reset();

            for(int i = 1; i < sumbkg->GetNbinsX(); i++){
                float pullvalue = 0.;

                if(sumbkg->GetBinError(i) != 0){          
                    pullvalue = (data[hists.first]->GetBinContent(i) - sumbkg->GetBinContent(i))/sumbkg->GetBinError(i);
                }

                pull->SetBinContent(i, pullvalue);
            }
           
		    canvas->cd();
		    pullpad->Draw();
		    pullpad->cd();
            pullpad->SetBottomMargin(0.2);
            pullpad->SetLeftMargin(0.15); 

            pull->SetMarkerStyle(20);
            pull->SetTickLength(0.14, "X");
            pull->SetLabelSize(0.12, "XY");
            pull->GetYaxis()->SetTitle("#frac{data - bkg}{Unc. of bkg}");
            pull->Draw("EP");  
        }
          
        mainpad->cd();
        
        float maximum = std::max(stack->GetMaximum(), data[hists.first]->GetMaximum());
        stack->SetMaximum(maximum*1.25); 

        for(std::string outdir: outdirs){
            canvas->SaveAs((outdir + "/" + hists.first + ".pdf").c_str());
            canvas->SaveAs((outdir + "/" + hists.first + ".png").c_str());
        }
        
        stack->SetMinimum(1e-1); 
        stack->SetMaximum(maximum*1e3); 
        mainpad->SetLogy();

        for(std::string outdir: outdirs){
            canvas->SaveAs((outdir + "/" + hists.first + "_log.pdf").c_str());
            canvas->SaveAs((outdir + "/" + hists.first + "_log.png").c_str());
        }

        std::cout << "Plot created for: " << hists.first << std::endl;
        
    }
}
