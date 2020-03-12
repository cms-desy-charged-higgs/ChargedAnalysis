#include <ChargedAnalysis/Analysis/include/plotter2D.h>

Plotter2D::Plotter2D() : Plotter(){}

Plotter2D::Plotter2D(std::string &histdir, std::vector<std::string> &xParameters, std::vector<std::string> &yParameters, std::string &channel, std::vector<std::string>& processes) :
    Plotter(histdir),
    xParameters(xParameters),
    yParameters(yParameters),
    channel(channel),
    processes(processes),
    background({}),
    signal({}),
    data({})
 {}


void Plotter2D::ConfigureHists(){
    for(unsigned int i = 0; i < xParameters.size(); i++){
        //Define Vector for processes
        std::vector<std::vector<TH2F*>> bkgHistVec;
        std::vector<std::vector<TH2F*>> sigHistVec;

        for(unsigned int j = 0; j < yParameters.size(); j++){
            std::vector<TH2F*> bkgHists;
            std::vector<TH2F*> sigHists;

            for(std::string process: processes){
                //Get Histogram for parameter
                std::string filename = histdir + "/" + process + ".root";
                TFile* file = TFile::Open(filename.c_str());
                
                //Get hist names because only non redundant plots are saved
                TList* histNames = file->GetListOfKeys();

                if(histNames->Contains((xParameters[i] + "_VS_" + yParameters[j]).c_str())){
                    TH2F* hist = (TH2F*)file->Get((xParameters[i] + "_VS_" + yParameters[j]).c_str());

                    if(Utils::Find<std::string>(process, "HPlus") != -1.){
                        hist->SetMarkerColor(kBlue);
                        hist->SetMarkerStyle(20);
                        sigHists.push_back(hist);
                    }

                    else{
                        hist->SetMarkerColor(kRed);
                        hist->SetMarkerStyle(20);
                        bkgHists.push_back(hist);
                    }
                }
                
                sigHistVec.push_back(sigHists);
                bkgHistVec.push_back(bkgHists);
            }
        }

        signal.push_back(sigHistVec);
        background.push_back(bkgHistVec);
    }
}

void Plotter2D::Draw(std::vector<std::string> &outdirs){
    Plotter::SetStyle();

    TCanvas *canvas = new TCanvas("canvas2D", "canvas2D", 1000, 800); 
    TPad* mainpad = new TPad("mainpad", "mainpad", 0., 0. , 1., 1.);

    //Save pairs of XY parameters to avoid redundant plots
   std::vector<std::string> parameterPairs;

    for(unsigned int i = 0; i < xParameters.size(); i++){
        for(unsigned int j = 0; j < background[i].size(); j++){
            canvas->cd();

            //Draw main pad
            Plotter::SetPad(mainpad);
            mainpad->SetRightMargin(0.15);
            mainpad->Draw();
            mainpad->cd();

            if(background[i][j].size() == 0) continue;

            TH2F* bkgSum =  (TH2F*)background[i][j][0]->Clone();
            bkgSum->Clear();

            for(TH2F* hist: background[i][j]){
                bkgSum->Add(hist);
            }
                       
            Plotter::SetHist(mainpad, bkgSum);
            bkgSum->DrawNormalized("colz");
            Plotter::DrawHeader(mainpad, channelHeader[channel], "Work in progress");

            for(std::string outdir: outdirs){
                canvas->SaveAs((outdir + "/" + std::string(background[i][j][0]->GetName()) + "_bkg.pdf").c_str());
                canvas->SaveAs((outdir + "/" + std::string(background[i][j][0]->GetName()) + "_bkg.png").c_str());
            }

            canvas->cd();

            //Draw main pad
            Plotter::SetPad(mainpad);
            mainpad->SetRightMargin(0.15);
            mainpad->Draw();
            mainpad->cd();

            if(signal[i][j].size() == 0) continue;

            Plotter::SetHist(mainpad, signal[i][j][0]);
            signal[i][j][0]->DrawNormalized("colz");
            Plotter::DrawHeader(mainpad, channelHeader[channel], "Work in progress");

            for(std::string outdir: outdirs){
                canvas->SaveAs((outdir + "/" + std::string(background[i][j][0]->GetName()) + "_sig.pdf").c_str());
                canvas->SaveAs((outdir + "/" + std::string(background[i][j][0]->GetName()) + "_sig.png").c_str());
            }

            std::cout << "Plot created for: " << std::string(background[i][j][0]->GetName()) << std::endl;
        }   
    }

    delete mainpad;
    delete canvas;
}
