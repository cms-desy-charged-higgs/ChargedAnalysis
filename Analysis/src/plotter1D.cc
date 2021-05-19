#include <ChargedAnalysis/Analysis/include/plotter1D.h>

Plotter1D::Plotter1D() : Plotter(){}

Plotter1D::Plotter1D(const std::string& channel, const std::string& era, const std::vector<std::string>& bkgProcesses, const std::map<std::string, std::vector<std::string>>& bkgFiles, const std::vector<std::string>& sigProcesses, const std::map<std::string, std::vector<std::string>>& sigFiles, const std::string& data, const std::string& dataFile, const std::vector<std::string> systematics) :
    Plotter(), 
    channel(channel), 
    era(era), 
    bkgProcesses(bkgProcesses), 
    bkgFiles(bkgFiles), 
    sigProcesses(sigProcesses), 
    sigFiles(sigFiles), 
    dataProcess(data), 
    dataFile(dataFile),
    systematics(systematics){}

void Plotter1D::ConfigureHists(){
    //Read out parameter
    std::shared_ptr<TFile> f = RUtil::Open(VUtil::Merge(sigFiles[""], bkgFiles[""]).at(0));

    for(int i = 0; i < f->GetListOfKeys()->GetSize(); ++i){
        if(f->Get(f->GetListOfKeys()->At(i)->GetName())->InheritsFrom(TH1F::Class())){
            parameters.push_back(f->GetListOfKeys()->At(i)->GetName());  
        }
    }

    for(const std::string& syst : systematics){
        for(const std::string shift : {"Up", "Down"}){
            //Skip Down loop for nominal case
            if(syst == "" and shift == "Down") continue;

            std::string systName = syst == "" ? "" : StrUtil::Merge(syst, shift);


            for(std::size_t i = 0; i < bkgProcesses.size(); ++i){
                std::shared_ptr<TFile> file = RUtil::Open(bkgFiles[systName].at(i));

                for(std::string& param: parameters){
                    if(param == "cutflow" and syst != "") continue;

                    std::shared_ptr<TH1F> hist = RUtil::GetSmart<TH1F>(file.get(), param);

                    if(syst == ""){
                        //Set Style
                        hist->SetFillStyle(1001);
                        hist->SetFillColor(colors.at(bkgProcesses.at(i)));
                        hist->SetLineColor(colors.at(bkgProcesses.at(i)));
                        hist->SetName(bkgProcesses.at(i).c_str());
                        hist->SetDirectory(0);

                        background[param].push_back(hist);                   

                        //Total bkg sum
                        if(bkgSum.count(param)) bkgSum[param]->Add(hist.get());
                        else{
                            bkgSum[param] = RUtil::CloneSmart(hist.get());
                            bkgSum[param]->SetDirectory(0);
                        }
                    }

                    else{
                        std::map<std::string, std::shared_ptr<TH1F>>& systShift = shift == "Up" ? systUp : systDown;
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
                    if(param == "cutflow" and syst != "") continue;

                    std::shared_ptr<TH1F> hist = RUtil::GetSmart<TH1F>(file.get(), param);

                    if(syst == ""){
                        std::vector<std::string> s = StrUtil::Split(sigProcesses.at(i), "_");

                        hist->SetLineWidth(1 + 3*signal[param].size());
                        hist->SetLineColor(kBlack);
                        hist->SetName(StrUtil::Replace("H^{#pm}_{[M]} + h_{[M]}", "[M]", s.at(0).substr(5), s.at(1).substr(1)).c_str());    
                        hist->SetDirectory(0);        

                        signal[param].push_back(hist);
                    }
                }
            }
        }
    }

    if(dataProcess != ""){
        std::shared_ptr<TFile> file = RUtil::Open(dataFile);

        for(std::string& param: parameters){
            std::shared_ptr<TH1F> hist = RUtil::GetSmart<TH1F>(file.get(), param);

            hist->SetMarkerStyle(20);
            hist->SetName("data");
            hist->SetDirectory(0);

            data[param] = hist;
        }
    }
}

void Plotter1D::Draw(std::vector<std::string> &outdirs){
    //Set overall style
    PUtil::SetStyle();

    for(std::string& param: parameters){
        //All canvases/pads
        std::shared_ptr<TCanvas> canvas = std::make_shared<TCanvas>("canvas",  "canvas", 1000, 1000);
        std::shared_ptr<TPad> mainPad = std::make_shared<TPad>("mainPad", "mainPad", 0., 0. , 1., 1.);

        //Draw main pad
        PUtil::SetPad(mainPad.get());
        mainPad->Draw();

        //Sort histograms by integral and fill to HStack
        std::shared_ptr<THStack> allBkgs = std::make_shared<THStack>();
        std::shared_ptr<TH1F> statUnc;

        if(background.count(param)){
            std::sort(background[param].begin(), background[param].end(), [&](std::shared_ptr<TH1F> h1, std::shared_ptr<TH1F> h2){return h1->Integral() < h2->Integral();});

            for(std::shared_ptr<TH1F>& hist: background[param]){allBkgs->Add(hist.get());}

            statUnc = RUtil::CloneSmart(bkgSum[param].get());
            statUnc->SetFillStyle(3354);
            statUnc->SetFillColorAlpha(kBlack, 0.8);
            statUnc->SetMarkerColor(kBlack);
        }

        //Rerverse sort histograms by integral and fill to Legend
        std::shared_ptr<TLegend> legend = std::make_shared<TLegend>(0., 0., 1, 1);

        if(data.count(param)) legend->AddEntry(data[param].get(), data[param]->GetName(), "EP");

        if(background.count(param)){
            std::sort(background[param].begin(), background[param].end(), [&](std::shared_ptr<TH1F> h1, std::shared_ptr<TH1F> h2){return h1->Integral() > h2->Integral();});
            for(std::shared_ptr<TH1F>& hist: background[param]){legend->AddEntry(hist.get(), hist->GetName(), "F");}
            legend->AddEntry(statUnc.get(), "Stat. unc.", "F");
        }

        if(signal.count(param)){
            for(std::shared_ptr<TH1F>& hist: signal[param]){legend->AddEntry(hist.get(), hist->GetName(), "L");}
        }

        //Frame hist
        std::shared_ptr<TH1F> frame = bkgSum.count(param) ? RUtil::CloneSmart(bkgSum[param].get()) : data.count(param) ? RUtil::CloneSmart(data[param].get()) : RUtil::CloneSmart(signal[param][0].get());
        frame->Reset();
        frame->LabelsDeflate();
        PUtil::SetHist(mainPad.get(), frame.get(), "Events");

        //Draw ratio
        if(bkgSum.count(param) and data.count(param)){
            PUtil::DrawRatio(canvas.get(), mainPad.get(), data[param].get(), bkgSum[param].get(), "Data/Pred");
        }

        for(int isLog=0; isLog < 2; ++isLog){
            mainPad->cd();
            mainPad->Clear();
            mainPad->SetLogy(isLog);

            int nLegendColumns = std::ceil((legend->GetNRows())/5.);

            frame->SetMinimum(isLog ? 1e-1 : 0);
            frame->Draw();

            //Draw data and MC
            if(background.count(param)){
                allBkgs->Draw("HIST SAME");
                statUnc->Draw("SAME E2");
            }

            if(data.count(param)) data[param]->Draw("E SAME");

            if(signal.count(param)){
                for(std::shared_ptr<TH1F>& hist: signal[param]){hist->Draw("HIST SAME");}
            }

            //Redraw axis
            gPad->RedrawAxis();

            //Draw Legend
            PUtil::DrawLegend(mainPad.get(), legend.get(), 5);

            //Draw Title
            PUtil::DrawHeader(mainPad.get(), PUtil::GetChannelTitle(channel), "Work in progress", PUtil::GetLumiTitle(era));

            //Save everything
            std::string extension = isLog ? "_log" : "";

            for(std::string outdir: outdirs){
                std::system(("mkdir -p " + outdir).c_str());
                canvas->SaveAs((outdir + "/" + param + extension + ".pdf").c_str());
                canvas->SaveAs((outdir + "/" + param + extension + ".png").c_str());

                std::cout << "Saved plot: '" + outdir + "/" + param + extension + ".pdf'" << std::endl;
            }
        }

        //Draw shape plots if signal is there
        if(signal.count(param) and param != "cutflow" and param != "EventCount"){
            for(std::shared_ptr<TH1F>& hist: signal[param]){
                std::shared_ptr<TCanvas> c = std::make_shared<TCanvas>("canvasSig",  "canvasSig", 1000, 1000);

                PUtil::DrawShapes(c.get(), statUnc.get(), hist.get());
                PUtil::DrawHeader(c.get(), PUtil::GetChannelTitle(channel), "Work in progress", PUtil::GetLumiTitle(era));

                std::string mass = std::string(hist->GetName()).substr(std::string(hist->GetName()).find("H^{#pm}_{") + 9, 3);

                for(std::string outdir: outdirs){
                    c->SaveAs((outdir + "/" + param + "_" + mass + "_shape.pdf").c_str());
                    c->SaveAs((outdir + "/" + param + "_" + mass + "_shape.png").c_str());

                    std::cout << "Saved plot: '" + outdir + "/" + param + "_" + mass + "_shape.pdf'" << std::endl;
                }
            }
        }

        //Draw systematics
        if(systematics.size() > 1){
            if(param == "cutflow") continue;

            for(const std::string shift : {"Up", "Down"}){
                std::shared_ptr<TCanvas> c = std::make_shared<TCanvas>("canvasSyst",  "canvasSyst", 1000, 1000);
                std::shared_ptr<TLegend> l = std::make_shared<TLegend>(0., 0., 1, 1);

                //Style stuff
                PUtil::SetStyle();
                PUtil::SetPad(c.get());

                float min = 0., max = 0.;

                for(const std::string& syst : systematics){
                    if(syst == "") continue;
                    std::string key = syst + param;

                    TH1F* relErr = RUtil::Clone(shift == "Up" ? systUp.at(key).get() : systDown.at(key).get());
                    relErr->Add(bkgSum[param].get(), -1);
                    relErr->Divide(bkgSum[param].get());
                    relErr->SetLineWidth(2);

                    if(l->GetListOfPrimitives()->GetSize() == 0){
                        PUtil::SetHist(c.get(), relErr, ("N^{" + shift + "}_{sys}- N_{nom})/N_{nom}").c_str());
                        relErr->SetMinimum(-0.07);
                        relErr->SetMaximum(0.07);
                        relErr->Draw("HIST PLC");
                    }

                    else relErr->Draw("HIST SAME PLC");

                    l->AddEntry(relErr, syst.c_str(), "L");
                }

                PUtil::DrawLegend(c.get(), l.get(), 5);
                PUtil::DrawHeader(c.get(), PUtil::GetChannelTitle(channel), "Work in progress", PUtil::GetLumiTitle(era));

                for(std::string outdir: outdirs){
                    std::system(("mkdir -p " + outdir).c_str());
                    c->SaveAs((outdir + "/" + param + "_systematics" + shift + ".pdf").c_str());
                    c->SaveAs((outdir + "/" + param + "_systematics" + shift + ".png").c_str());

                    std::cout << "Saved plot: '" + outdir + "/" + param + "_systematics" + shift + ".pdf'" << std::endl;
                }   
            }
        }
    }
}
