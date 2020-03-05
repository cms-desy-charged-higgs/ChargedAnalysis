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
                parameters.push_back(file->GetListOfKeys()->At(i)->GetName());
            }
        }

        for(std::string& param: parameters){
            TH1F* hist = (TH1F*)file->Get(param.c_str());

            if(hist == NULL){
                throw std::runtime_error("Did not found histogram '" + param + "' in file '" + fileName + "'");
            }

            //Data configuration
            if(Utils::Find<std::string>(process, "Single") != -1.){
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

        if(background.count(param)){
            std::sort(background[param].begin(), background[param].end(), [](TH1F* h1, TH1F* h2){return h1->Integral() < h2->Integral();});

            for(TH1F* hist: background[param]){allBkgs->Add(hist);}
        }

        //Rerverse sort histograms by integral and fill to Legend
        TLegend* legend = new TLegend(0., 0., 1, 1);

        if(data.count(param)) legend->AddEntry(data[param], data[param]->GetName(), "EP");

        if(background.count(param)){
            std::sort(background[param].begin(), background[param].end(), [](TH1F* h1, TH1F* h2){return h1->Integral() > h2->Integral();});
            for(TH1F* hist: background[param]){legend->AddEntry(hist, hist->GetName(), "F");}
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

            frame->SetMinimum(isLog ? 1e-1 : 0);
            frame->SetMaximum(isLog ? max*1e1 : max*1.15);
            frame->Draw();

            //Draw Title
            Plotter::DrawHeader(mainPad, channelHeader[channel], "Work in progress");

            //Draw data and MC
            if(background.count(param)) allBkgs->Draw("HIST SAME");
            if(data.count(param)) data[param]->Draw("E SAME");

            if(signal.count(param)){
                for(TH1F* hist: signal[param]){hist->Draw("HIST SAME");}
            }

            //Redraw axis
            gPad->RedrawAxis();

            //Draw Legend
            Plotter::DrawLegend(legend, (background.count(param) ? background[param].size() : 0) +
                                        (signal.count(param) ? signal[param].size() : 0) + 
                                        data.count(param)
            );

            //Save everything
            std::string extension = isLog ? "_log" : "";

            for(std::string outdir: outdirs){
                canvas->SaveAs((outdir + "/" + param + extension + ".pdf").c_str());
                canvas->SaveAs((outdir + "/" + param + extension + ".png").c_str());

                std::cout << "Saved plot: '" + outdir + "/" + param + extension + ".pdf'" << std::endl;
            }
        }

        delete legend; delete allBkgs; delete mainPad; delete canvas;
    }
}
