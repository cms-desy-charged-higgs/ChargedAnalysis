#include <ChargedAnalysis/Analysis/include/plotter1D.h>

Plotter1D::Plotter1D() : Plotter(){}

Plotter1D::Plotter1D(std::string &histdir, std::string &channel, std::vector<std::string> &processes) :
    Plotter(histdir), 
    channel(channel),
    processes(processes){}

void Plotter1D::ConfigureHists(){
    for(std::string process: processes){
        std::string fileName = histdir + "/" + process + "/" + process + ".root";
        TFile* file = TFile::Open(fileName.c_str());

        std::cout << "Read histograms from file: '" + fileName + "'" << std::endl;

        if(parameters.empty()){
            for(int i = 0; i < file->GetListOfKeys()->GetSize(); i++){
                if(file->Get(file->GetListOfKeys()->At(i)->GetName())->InheritsFrom(TH1F::Class())){
                    parameters.push_back(file->GetListOfKeys()->At(i)->GetName());  
                }
            }
        }

        for(std::string& param: parameters){
            TH1F* hist = file->Get<TH1F>(param.c_str());
  
            if(hist == NULL){
                throw std::runtime_error("Did not found histogram '" + param + "' in file '" + fileName + "'");
            }

            //Data configuration
            if(Utils::Find<std::string>(process, "Single") != -1. or Utils::Find<std::string>(process, "JetHT") != -1.){
                hist->SetMarkerStyle(20);
                hist->SetName("data");

                data[param] = hist;
            }

            //Signal configuration
            else if(Utils::Find<std::string>(process, "HPlus") != -1.){
                hist->SetLineWidth(1 + 3*signal[param].size());
                hist->SetLineColor(kBlack);

                std::vector<std::string> massStrings = Utils::SplitString<std::string>(process, "_");

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

                if(bkgSum.count(param)) bkgSum[param]->Add(hist);
                else bkgSum[param] = (TH1F*)hist->Clone();
            }
        }
    }
}

void Plotter1D::Draw(std::vector<std::string> &outdirs){
    //Set overall style
    Plotter::SetStyle();

    for(std::string& param: parameters){
        //All canvases/pads
        TCanvas* canvas = new TCanvas("canvas",  "canvas", 1000, 1000);
        TPad* mainPad = new TPad("mainPad", "mainPad", 0., 0. , 1., 1.);

        //Draw main pad
        Plotter::SetPad(mainPad);
        mainPad->Draw();

        //Get max value
        float max = std::max({
            data.count(param) ? data[param]->GetMaximum() : 0,
            bkgSum.count(param) ? bkgSum[param]->GetMaximum() : 0,
            signal.count(param) ? signal[param][0]->GetMaximum() : 0,
        });

        //Sort histograms by integral and fill to HStack
        THStack* allBkgs = new THStack();
        TH1F* statUnc;

        if(background.count(param)){
            std::sort(background[param].begin(), background[param].end(), [](TH1F* h1, TH1F* h2){return h1->Integral() < h2->Integral();});

            for(TH1F* hist: background[param]){allBkgs->Add(hist);}

            statUnc = (TH1F*)bkgSum[param]->Clone();
            statUnc->SetFillStyle(3354);
            statUnc->SetFillColorAlpha(kBlack, 0.8);
            statUnc->SetMarkerColor(kBlack);
        }

        //Rerverse sort histograms by integral and fill to Legend
        TLegend* legend = new TLegend(0., 0., 1, 1);

        if(data.count(param)) legend->AddEntry(data[param], data[param]->GetName(), "EP");

        if(background.count(param)){
            std::sort(background[param].begin(), background[param].end(), [](TH1F* h1, TH1F* h2){return h1->Integral() > h2->Integral();});
            for(TH1F* hist: background[param]){legend->AddEntry(hist, hist->GetName(), "F");}
            legend->AddEntry(statUnc, "Stat. unc.", "F");
        }

        if(signal.count(param)){
            for(TH1F* hist: signal[param]){legend->AddEntry(hist, hist->GetName(), "L");}
        }

        //Frame hist
        TH1F* frame = bkgSum.count(param) ? (TH1F*)bkgSum[param]->Clone() : data.count(param) ? (TH1F*)data[param]->Clone() :
        (TH1F*)signal[param][0]->Clone();
        frame->Reset();
        Plotter::SetHist(mainPad, frame, "Events");

        //Draw ratio
        if(bkgSum.count(param) and data.count(param)){
            Plotter::DrawRatio(canvas, mainPad, data[param], bkgSum[param], "Data/Pred");
        }

        for(int isLog=0; isLog < 2; isLog++){
            mainPad->cd();
            mainPad->Clear();
            mainPad->SetLogy(isLog);

            int nLegendColumns = std::ceil((legend->GetNRows())/5.);

            frame->SetMinimum(isLog ? 1e-1 : 0);
            frame->SetMaximum(isLog ? max*std::pow(10, 2*nLegendColumns) : max*(1 + nLegendColumns*0.1));
            frame->Draw();

            //Draw Title
            Plotter::DrawHeader(mainPad, channelHeader[channel], "Work in progress");

            //Draw data and MC
            if(background.count(param)){
                allBkgs->Draw("HIST SAME");
                statUnc->Draw("SAME E2");
            }

            if(data.count(param)) data[param]->Draw("E SAME");

            if(signal.count(param)){
                for(TH1F* hist: signal[param]){hist->Draw("HIST SAME");}
            }

            //Redraw axis
            gPad->RedrawAxis();

            //Draw Legend
            Plotter::DrawLegend(legend, 5);

            //Save everything
            std::string extension = isLog ? "_log" : "";

            for(std::string outdir: outdirs){
                std::system(("mkdir -p " + outdir).c_str());
                canvas->SaveAs((outdir + "/" + param + extension + ".pdf").c_str());
                canvas->SaveAs((outdir + "/" + param + extension + ".png").c_str());

                std::cout << "Saved plot: '" + outdir + "/" + param + extension + ".pdf'" << std::endl;
            }
        }

        delete legend; delete mainPad, delete canvas;

        //Draw shape plots if signal is there
        if(signal.count(param) and param != "cutflow"){
            for(TH1F* hist: signal[param]){
                TCanvas* c = new TCanvas("canvas",  "canvas", 1000, 1000);

                Plotter::DrawShapes(c, statUnc, hist);
                Plotter::DrawHeader(c, channelHeader[channel], "Work in progress");

                std::string mass = std::string(hist->GetName()).substr(std::string(hist->GetName()).find("H^{#pm}_{") + 9, 3);

                for(std::string outdir: outdirs){
                    c->SaveAs((outdir + "/" + param + "_" + mass + "_shape.pdf").c_str());
                    c->SaveAs((outdir + "/" + param + "_" + mass + "_shape.png").c_str());

                    std::cout << "Saved plot: '" + outdir + "/" + param + "_" + mass + "_shape.pdf'" << std::endl;
                }

                delete c;
            }
        }
    }
}
