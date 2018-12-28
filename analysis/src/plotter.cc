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
    }),
    xLabels({
        {"W_pt", "p_{T}(W) [GeV]"},
        {"W_phi", "#phi(W) [rad]"},
        {"W_mT", "m_{T}(W) [GeV]"},
        {"e_pt", "p_{T}(e) [GeV]"},
        {"nJet", "Number of Jets"},
        {"j1_pt", "p_{T}(j_{1}) [GeV]"},
    })
 {}

void Plotter::ConfigureHists(std::vector<std::string> &parameters, std::vector<std::string> &processes){
    for(std::string process: processes){
        std::string filename = histdir + "/" + process + ".root";

        TFile* file = TFile::Open(filename.c_str());

        for(std::string parameter: parameters){
            TH1F* hist = 0;

            file->GetObject((parameter + "_" + process).c_str(), hist);
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

void Plotter::Draw(){
    gStyle->SetLegendBorderSize(0);
    gStyle->SetFillStyle(0);

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

    std::function<bool(TH1F*,TH1F*)> sortFunc = [](TH1F* hist1, TH1F* hist2){return hist1->GetIntegral() > hist2->GetIntegral();};

    for(std::pair<std::string, std::vector<TH1F*>> hists: background){
        TCanvas* canvas = new TCanvas();
        THStack* stack = new THStack();
        TLegend* legend = new TLegend(0.65, 0.65, 0.9, 0.9);
       
        std::sort(hists.second.begin(), hists.second.end(), sortFunc);

        for(TH1F* hist: hists.second){
            legend->AddEntry(hist, hist->GetName(), "F");
            stack->Add(hist);
        }
        
        stack->Draw("HIST");
        stack->GetXaxis()->SetTitle(xLabels[hists.first].c_str());
        stack->GetYaxis()->SetTitle("Events");

        if(!data.empty()){
            legend->AddEntry(data[hists.first], data[hists.first]->GetName(), "P");
            data[hists.first]->Draw("SAME EP");
        }

        legend->Draw("SAME");

        //channel_title->DrawLatexNDC(0.17, 0.905, "e4bjets");
        cms->DrawLatexNDC(0.25, 0.905, "CMS");
        work->DrawLatexNDC(0.318, 0.905, "Work in progress");
        lumi->DrawLatexNDC(0.605, 0.905, "41.4 fb^{-1} (2017, 13 TeV)");

        canvas->SaveAs((hists.first + ".pdf").c_str());
        
        stack->SetMinimum(1e-1); 
        stack->SetMaximum(1e6); 
        canvas->SetLogy();

        canvas->SaveAs((hists.first + "_log.pdf").c_str());
    }
}
