#include <ChargedAnalysis/Analysis/include/plotterPostfit.h>

PlotterPostfit::PlotterPostfit() : Plotter(){}

PlotterPostfit::PlotterPostfit(const std::string& inFile, const std::vector<std::string>& bkgProcesses, const std::string&  sigProcess) : 
    Plotter(),
    inFile(inFile),
    sigProcess(sigProcess),
    bkgProcesses(bkgProcesses){}

void PlotterPostfit::ConfigureHists(){
    std::shared_ptr<TFile> file = RUtil::Open(inFile);
    int nBins = 0;

    for(TObject* dir : *RUtil::Get<TDirectory>(file.get(), "shapes_fit_s")->GetListOfKeys()){
        ++nBins;
        std::string dirName(dir->GetName());

        //Bkg hists
        for(const std::string& bkg : bkgProcesses){
            std::shared_ptr<TH1F> hist;

            try{
                hist = RUtil::GetSmart<TH1F>(file.get(), StrUtil::Join("/", "shapes_fit_s", dirName, bkg));
            }

            catch(...){
                std::cout << "Histogram for process '" << bkg << "' not existing, probably due to empty entry in the datacard!" << std::endl;
                continue;
            }

            if(!backgrounds.count(bkg)){
                backgrounds[bkg] = std::make_shared<TH1F>(bkg.c_str(), bkg.c_str(), 1, 1, 1);

                //Set Style
                backgrounds[bkg]->SetFillStyle(1001);
                backgrounds[bkg]->SetFillColor(colors.at(bkg));
                backgrounds[bkg]->SetLineColor(colors.at(bkg));
                backgrounds[bkg]->SetDirectory(0);
            }

            backgrounds[bkg]->Fill(dirName.c_str(), hist->Integral());
        }

        //Signal hist
        std::shared_ptr<TH1F> sig = RUtil::GetSmart<TH1F>(file.get(), StrUtil::Join("/", "shapes_fit_s", dirName, sigProcess));

        if(signal == nullptr){
            std::vector<std::string> s = StrUtil::Split(sigProcess, "_");
            signal = std::make_shared<TH1F>("sig", "sig", 1, 1, 1);

            signal->SetLineWidth(3);
            signal->SetLineColor(kBlack);
            signal->SetName(StrUtil::Replace("H^{#pm}_{[M]} + h_{[M]}", "[M]", s.at(0).substr(5), s.at(1).substr(1)).c_str());    
            signal->SetDirectory(0);
        }

        signal->Fill(dirName.c_str(), sig->Integral());

        //Data Graph
        std::shared_ptr<TGraphAsymmErrors> d = RUtil::GetSmart<TGraphAsymmErrors>(file.get(), StrUtil::Join("/", "shapes_fit_s", dirName, "data"));

        if(data == nullptr){
            data = std::make_shared<TGraphAsymmErrors>();
            data->SetMarkerStyle(20);    
        }

        data->SetPoint(data->GetN(), 0.5 + data->GetN(), d->GetPointY(0));

        //Error band
        std::shared_ptr<TH1F> allBkg = RUtil::GetSmart<TH1F>(file.get(), StrUtil::Join("/", "shapes_fit_s", dirName, "total_background"));

        if(errorBand == nullptr){
            errorBand = std::make_shared<TH1F>("error", "error", 1, 1, 1);
            errorBand->SetFillStyle(3354);
            errorBand->SetFillColorAlpha(kBlack, 0.8);
            errorBand->SetMarkerColor(kBlack);
        }

        errorBand->Fill(dirName.c_str(), allBkg->GetBinContent(1));
        errorBand->SetBinError(nBins, allBkg->GetBinError(1));
    }
}

void PlotterPostfit::Draw(std::vector<std::string> &outdirs){
    //Set Style
    PUtil::SetStyle();

    //Define canvas and pads
    std::shared_ptr<TCanvas> c = std::make_shared<TCanvas>("canvas",  "canvas", 1000, 1000);
    std::shared_ptr<TPad> mainPad = std::make_shared<TPad>("mainPad", "mainPad", 0., 0. , 1., 1.);
    errorBand->LabelsDeflate();

    //Pad style
    PUtil::SetPad(mainPad.get());
    mainPad->Draw();
    mainPad->cd();

    PUtil::SetHist(mainPad.get(), errorBand.get(), "Events");

    //Create background stack
    std::vector<std::shared_ptr<TH1F>> bkgs = VUtil::MapValues(backgrounds);
    std::sort(bkgs.begin(), bkgs.end(), [](std::shared_ptr<TH1F> h1, std::shared_ptr<TH1F> h2){return h1->Integral() < h2->Integral();});

    std::shared_ptr<THStack> stack = std::make_shared<THStack>();

    for(std::shared_ptr<TH1F>& h : bkgs) stack->Add(h.get());

    //Legend
    std::shared_ptr<TLegend> legend = std::make_shared<TLegend>(0., 0., 1, 1);

    std::sort(bkgs.begin(), bkgs.end(), [&](std::shared_ptr<TH1F> h1, std::shared_ptr<TH1F> h2){return h1->Integral() > h2->Integral();});
    for(std::shared_ptr<TH1F>& hist: bkgs){legend->AddEntry(hist.get(), hist->GetName(), "F");}

    legend->AddEntry(errorBand.get(), "Syst. unc.", "F");
    legend->AddEntry(data.get(), "Data", "P");

    for(int isLog=0; isLog < 2; ++isLog){
        mainPad->cd();
        mainPad->Clear();
        mainPad->SetLogy(isLog);

        errorBand->SetMinimum(isLog ? 1e-1 : 0);

        errorBand->Draw("E2");
        stack->Draw("HIST SAME");
        data->Draw("EP SAME");
        signal->Draw("HIST SAME");
        errorBand->Draw("SAME E2");

        PUtil::DrawLegend(mainPad.get(), legend.get(), 5);
        PUtil::DrawHeader(mainPad.get(), "", "Work in progress", "");

        gPad->RedrawAxis();

        std::string extension = isLog ? "_log" : "";

        for(std::string outdir: outdirs){
            std::system(("mkdir -p " + outdir).c_str());
            c->SaveAs((outdir + "/postfit" + extension + ".pdf").c_str());
            c->SaveAs((outdir + "/postfit" + extension + ".png").c_str());
        }
    }
}
