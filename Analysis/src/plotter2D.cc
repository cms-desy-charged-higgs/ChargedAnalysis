#include <ChargedAnalysis/Analysis/include/plotter2D.h>

Plotter2D::Plotter2D() : Plotter(){}

Plotter2D::Plotter2D(std::string &histdir, std::string &channel, std::vector<std::string> &processes, const std::string& era) :
    Plotter(histdir), 
    channel(channel),
    processes(processes),
    era(era){}

void Plotter2D::ConfigureHists(){
    /*
    //Read out parameter
    std::shared_ptr<TFile> f = RUtil::Open(VUtil::Merge(sigFiles[""], bkgFiles[""]).at(0));

    for(int i = 0; i < f->GetListOfKeys()->GetSize(); ++i){
        if(f->Get(f->GetListOfKeys()->At(i)->GetName())->InheritsFrom(TH2F::Class())){
            parameters.push_back(f->GetListOfKeys()->At(i)->GetName());  
        }
    }

    for(std::size_t i = 0; i < processes.size(); ++i){
        std::shared_ptr<TFile> file = RUtil::Open(bkgFiles.at(i));

        for(std::string& param: parameters){
            std::shared_ptr<TH2F> hist = RUtil::GetSmart<TH2F>(file.get(), param);
        }
    }
    */
}

void Plotter2D::Draw(std::vector<std::string> &outdirs){
    PUtil::SetStyle();

    //Save pairs of XY parameters to avoid redundant plots
    for(std::string& param: parameters){
        TCanvas *canvas = new TCanvas("canvas2D", "canvas2D", 1000, 1000); 
        TPad* mainPad = new TPad("mainpad", "mainpad", 0., 0. , 1., 1.);

        for(std::string outdir: outdirs) std::system(("mkdir -p " + outdir).c_str());

        //Draw main pad
        PUtil::SetPad(mainPad);
        mainPad->SetRightMargin(0.15);
        mainPad->Draw();
        mainPad->cd();
    
        if(background.count(param)){
            TH2F* bkgSum =  (TH2F*)background[param][0]->Clone();
            bkgSum->Clear();

            for(TH2F* hist: background[param]){
                bkgSum->Add(hist);
            }

            PUtil::SetHist(mainPad, bkgSum);
            bkgSum->DrawNormalized("CONT4Z");    
            PUtil::DrawHeader(mainPad, PUtil::GetChannelTitle(channel), "Work in progress");

            for(std::string outdir: outdirs){
                canvas->SaveAs((outdir + "/" + param + "_bkg.pdf").c_str());
                canvas->SaveAs((outdir + "/" + param + "_bkg.png").c_str());

                std::cout << "Saved plot: '" + outdir + "/" + param  + "_bkg.pdf'" << std::endl;
            }
        }

        if(signal.count(param)){
            for(TH2F* hist: signal[param]){
                mainPad->Clear();
                PUtil::SetHist(mainPad, hist);

                std::string mass = std::string(hist->GetName()).substr(std::string(hist->GetName()).find("H^{#pm}_{") + 9, 3);

                hist->DrawNormalized("CONT4Z");
                PUtil::DrawHeader(mainPad, PUtil::GetChannelTitle(channel), "Work in progress", PUtil::GetLumiTitle(era));

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
