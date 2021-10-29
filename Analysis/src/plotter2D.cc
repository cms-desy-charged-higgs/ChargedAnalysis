#include <ChargedAnalysis/Analysis/include/plotter2D.h>

Plotter2D::Plotter2D(){}

Plotter2D::Plotter2D(const std::string& channel, const std::string& era, const std::vector<std::string>& bkgProcesses, const std::map<std::string, std::vector<std::string>>& bkgFiles, const std::vector<std::string>& sigProcesses, const std::map<std::string, std::vector<std::string>>& sigFiles, const std::string& data, const std::string& dataFile, const std::vector<std::string> systematics) :
    channel(channel), 
    era(era), 
    bkgProcesses(bkgProcesses), 
    bkgFiles(bkgFiles), 
    sigProcesses(sigProcesses), 
    sigFiles(sigFiles), 
    dataProcess(data), 
    dataFile(dataFile),
    systematics(systematics){}

void Plotter2D::ConfigureHists(){
    //Read out parameter
    std::shared_ptr<TFile> f = RUtil::Open(VUtil::Merge(sigFiles["Nominal"], bkgFiles["Nominal"]).at(0));

    for(const std::string& param : RUtil::ListOfContent(f.get())){
        if(f->Get(param.c_str())->InheritsFrom(TH2F::Class())){
            parameters.push_back(param);  
        }
    }

    for(const std::string& syst : systematics){
        for(const std::string shift : {"Up", "Down"}){
            //Skip Down loop for nominal case
            if(syst == "Nominal" and shift == "Down") continue;

            std::string systName = syst == "Nominal" ? "Nominal" : StrUtil::Merge(syst, shift);

            for(std::size_t i = 0; i < bkgProcesses.size(); ++i){
                std::shared_ptr<TFile> file = RUtil::Open(bkgFiles[systName].at(i));
                std::shared_ptr<TH2F> hist;

                for(std::string& param: parameters){
                    try{
                        hist = RUtil::GetSmart<TH2F>(file.get(), param);
                    }
    
                    catch(...){continue;}

                    if(syst == "Nominal"){
                        //Set Style
                        hist->SetName(bkgProcesses.at(i).c_str());

                        background[param].push_back(hist);       
                        background[param].back()->SetDirectory(0);        

                        //Total bkg sum
                        if(bkgSum.count(param)) bkgSum[param]->Add(hist.get());
                        else{
                            bkgSum[param] = RUtil::CloneSmart(hist.get());
                            bkgSum[param]->SetDirectory(0);
                        }
                    }

                    else{
                        std::map<std::string, std::shared_ptr<TH2F>>& systShift = shift == "Up" ? systUp : systDown;
                        std::string key = syst + param;

                        if(systShift.count(key)) systShift[key]->Add(hist.get());

                        else{
                            systShift[key] = RUtil::CloneSmart(hist.get());
                            systShift[key]->SetDirectory(0);
                        }
                    }
                }
            }

            for(std::size_t i = 0; i < sigProcesses.size(); ++i){
                std::shared_ptr<TFile> file = RUtil::Open(sigFiles[systName].at(i));

                for(std::string& param: parameters){
                    std::shared_ptr<TH2F> hist = RUtil::GetSmart<TH2F>(file.get(), param);

                    if(syst == "Nominal"){
                        std::vector<std::string> s = StrUtil::Split(sigProcesses.at(i), "_");
                        hist->SetName(StrUtil::Replace("H^{#pm}_{[M]} + h_{[M]}", "[M]", s.at(0).substr(5), s.at(1).substr(1)).c_str());    

                        signal[param].push_back(hist);
                        signal[param].back()->SetDirectory(0);        
                    }
                }
            }
        }
    }

    if(dataProcess != ""){
        std::shared_ptr<TFile> file = RUtil::Open(dataFile);

        for(std::string& param: parameters){
            std::shared_ptr<TH2F> hist = RUtil::GetSmart<TH2F>(file.get(), param);

            hist->SetName("data");

            data[param] = hist;
            data[param]->SetDirectory(0);
        }
    }
}

void Plotter2D::Draw(std::vector<std::string> &outdirs){
    PUtil::SetStyle();

    //Save pairs of XY parameters to avoid redundant plots
    for(std::string& param: parameters){
        std::shared_ptr<TCanvas> canvas = std::make_shared<TCanvas>("canvas2D", "canvas2D", 1000, 1000); 
        std::shared_ptr<TPad> mainPad = std::make_shared<TPad>("mainpad", "mainpad", 0., 0. , 1., 1.);

        for(std::string outdir: outdirs) std::system(("mkdir -p " + outdir).c_str());

        //Draw main pad
        PUtil::SetPad(mainPad.get());
        mainPad->SetRightMargin(0.15);
        mainPad->Draw();
        mainPad->cd();
    
        if(background.count(param)){
            for(std::shared_ptr<TH2F>& hist: background[param]){
                mainPad->Clear();

                PUtil::SetHist(mainPad.get(), hist.get());
                hist->DrawNormalized("COLZ");    
                PUtil::DrawHeader(mainPad.get(), PUtil::GetChannelTitle(channel), "Work in progress");

                for(std::string outdir: outdirs){
                    std::string outName = StrUtil::Merge(outdir, "/", param, "_", hist->GetName());

                    canvas->SaveAs((outName + ".pdf").c_str());
                    canvas->SaveAs((outName + ".png").c_str());

                    std::cout << "Saved plot: '" + outName + ".pdf'" << std::endl;
                }
            }

            mainPad->Clear();
            PUtil::SetHist(mainPad.get(), bkgSum[param].get());
            bkgSum[param]->DrawNormalized("COLZ");    
            PUtil::DrawHeader(mainPad.get(), PUtil::GetChannelTitle(channel), "Work in progress");

            for(std::string outdir: outdirs){
                canvas->SaveAs((outdir + "/" + param + "_allBkg.pdf").c_str());
                canvas->SaveAs((outdir + "/" + param + "_allBkg.png").c_str());

                std::cout << "Saved plot: '" + outdir + "/" + param  + "_allBkg.pdf'" << std::endl;
            }
        }

        if(signal.count(param)){
            for(std::shared_ptr<TH2F>& hist: signal[param]){
                mainPad->Clear();
                PUtil::SetHist(mainPad.get(), hist.get());

                hist->DrawNormalized("COLZ");
                PUtil::DrawHeader(mainPad.get(), PUtil::GetChannelTitle(channel), "Work in progress", PUtil::GetLumiTitle(era));

                for(std::string outdir: outdirs){
                    std::string outName = StrUtil::Merge(outdir, "/", param, "_", hist->GetName());

                    canvas->SaveAs((outName + ".pdf").c_str());
                    canvas->SaveAs((outName + ".png").c_str());

                    std::cout << "Saved plot: '" + outName + ".pdf'" << std::endl;
                }
            }
        }

        if(data.count(param)){
            mainPad->Clear();
            PUtil::SetHist(mainPad.get(), data[param].get());

            data[param]->DrawNormalized("COLZ");
            PUtil::DrawHeader(mainPad.get(), PUtil::GetChannelTitle(channel), "Work in progress", PUtil::GetLumiTitle(era));

            for(std::string outdir: outdirs){
                std::string outName = StrUtil::Merge(outdir, "/", param, "_", "data");

                canvas->SaveAs((outName + ".pdf").c_str());
                canvas->SaveAs((outName + ".png").c_str());

                std::cout << "Saved plot: '" + outName + ".pdf'" << std::endl;
            }
        }
    }
}
