#include <ChargedAnalysis/Analysis/include/plotter2D.h>

Plotter2D::Plotter2D() : Plotter(){}

Plotter2D::Plotter2D(std::string &histdir, std::string &channel, std::vector<std::string> &processes) :
    Plotter(histdir), 
    channel(channel),
    processes(processes){}

void Plotter2D::ConfigureHists(){
    for(std::string process: processes){
        std::string fileName = histdir + "/" + process + "/" + process + ".root";
        TFile* file = TFile::Open(fileName.c_str());

        std::cout << "Read histograms from file: '" + fileName + "'" << std::endl;

        if(parameters.empty()){
            for(int i = 0; i < file->GetListOfKeys()->GetSize(); i++){
                if(file->Get(file->GetListOfKeys()->At(i)->GetName())->InheritsFrom(TH2F::Class())){
                    parameters.push_back(file->GetListOfKeys()->At(i)->GetName());  
                }
            }
        }

        for(std::string& param: parameters){
            TH2F* hist = file->Get<TH2F>(param.c_str());  

            if(Utils::Find<std::string>(process, "HPlus") != -1.){
                std::vector<std::string> massStrings = Utils::SplitString<std::string>(process, "_");
                hist->SetName(("H^{#pm}_{" + massStrings[0].substr(5,7) + "}+h_{" + massStrings[1].substr(1,3) + "}").c_str());

                signal[param].push_back(hist);
            }

            else{       
                hist->SetName(process.c_str());            
                background[param].push_back(hist);
            }   
        }
    }
}

void Plotter2D::Draw(std::vector<std::string> &outdirs){
    Plotter::SetStyle();

    //Save pairs of XY parameters to avoid redundant plots
    for(std::string& param: parameters){
        TCanvas *canvas = new TCanvas("canvas2D", "canvas2D", 1000, 1000); 
        TPad* mainPad = new TPad("mainpad", "mainpad", 0., 0. , 1., 1.);

        for(std::string outdir: outdirs) std::system(("mkdir -p " + outdir).c_str());

        //Draw main pad
        Plotter::SetPad(mainPad);
        mainPad->SetRightMargin(0.15);
        mainPad->Draw();
        mainPad->cd();
    
        if(background.count(param)){
            TH2F* bkgSum =  (TH2F*)background[param][0]->Clone();
            bkgSum->Clear();

            for(TH2F* hist: background[param]){
                bkgSum->Add(hist);
            }

            Plotter::SetHist(mainPad, bkgSum);
            bkgSum->DrawNormalized("CONT4Z");    
            Plotter::DrawHeader(mainPad, channelHeader[channel], "Work in progress");

            for(std::string outdir: outdirs){
                canvas->SaveAs((outdir + "/" + param + "_bkg.pdf").c_str());
                canvas->SaveAs((outdir + "/" + param + "_bkg.png").c_str());

                std::cout << "Saved plot: '" + outdir + "/" + param  + "_bkg.pdf'" << std::endl;
            }
        }

        if(signal.count(param)){
            for(TH2F* hist: signal[param]){
                mainPad->Clear();
                Plotter::SetHist(mainPad, hist);

                std::string mass = std::string(hist->GetName()).substr(std::string(hist->GetName()).find("H^{#pm}_{") + 9, 3);

                hist->DrawNormalized("CONT4Z");
                Plotter::DrawHeader(mainPad, channelHeader[channel], "Work in progress");

                for(std::string outdir: outdirs){
                    canvas->SaveAs((outdir + "/" + param + "_" + mass + "_sig.pdf").c_str());
                    canvas->SaveAs((outdir + "/" + param + "_" + mass + "_sig.png").c_str());

                    std::cout << "Saved plot: '" + outdir + "/" + param + "_" + mass + "_sig.pdf'" << std::endl;
                }
            }
        }

        delete mainPad;
        delete canvas;
    }
}
