#include <ChargedAnalysis/Analysis/include/QCDestimator.h>

QCDEstimator::QCDEstimator(const std::vector<std::string>& processes, const std::vector<std::string>& regions, const std::map<std::pair<std::string, std::string>, std::string>& inputFiles, const std::string& inputTF) :
    processes(processes),
    regions(regions),
    inputTF(inputTF),
    inputFiles(inputFiles){}

std::shared_ptr<TH1D> QCDEstimator::CalcTF(std::map<std::string, std::shared_ptr<TH1D>>& yields1D){
    //Initial tf with fine binning
    std::shared_ptr<TH1D> tf = RUtil::CloneSmart(yields1D.at("B").get());
    tf->Divide(yields1D.at("A").get());

    //Error threshold per bin
    double errThr = 0.3;

    while(true){
        //Get current binning
        unsigned int nX;
        std::vector<double> binning(tf->GetNbinsX() + 1, 0.);

        for(unsigned int i = 1; i <= tf->GetNbinsX() + 1; ++i){
            binning[i - 1] = tf->GetXaxis()->GetBinLowEdge(i);
        }

        nX = binning.size();

        //Rebin (by remove next to last bin boundary) while err threshold not fit (or only 1 bin left)
        for(unsigned int bin = binning.size() - 1; bin > 1; --bin){
            if(std::abs(tf->GetBinError(bin)) > errThr or std::isnan(tf->GetBinError(bin)) or tf->GetBinError(bin) <= 0){
                binning.erase(binning.begin() + bin - 1);
                bin = binning.size() - 1;

                for(const std::string& region : regions){
                    yields1D[region].reset(static_cast<TH1D*>(yields1D[region]->Rebin(binning.size() - 1, "", binning.data())));
                    yields1D[region]->SetDirectory(0);
                }

                tf.reset(static_cast<TH1D*>(tf->Rebin(binning.size() - 1, "", binning.data())));
                tf->Divide(yields1D.at("B").get(), yields1D.at("A").get());
            }
        }

        //No rebinning happened, leave rebinning loop
        if(nX == binning.size()) break;
    }

    tf->SetDirectory(0);
    tf->SetName("tf");
    tf->SetTitle("tf");

    return tf;
}

void QCDEstimator::Estimate(const std::string& outName, const std::vector<double>& bins, const bool& onlyTF){
    //Open data/background files
    std::map<std::pair<std::string, std::string>, std::shared_ptr<TFile>> files;

    for(const std::string region : regions){
        for(const std::string& process : processes){
            files[{region, process}] = RUtil::Open(inputFiles.at({region, process}));
        }
    }

    //Open output file
    std::shared_ptr<TFile> outFile = std::make_shared<TFile>(outName.c_str(), "RECREATE");
    std::shared_ptr<TH1F> eventCount;

    //1D histogram for tf
    std::shared_ptr<TH1D> tf = inputTF == "" ? nullptr : RUtil::GetSmart<TH1D>(RUtil::Open(inputTF).get(), "tf");
    std::vector<double> binningY;

    //List of variables
    for(const std::string varName : RUtil::ListOfContent(files.at({regions.at(0), processes.at(0)}).get())){
        if(files.at({regions.at(0), processes.at(0)})->Get(varName.c_str())->InheritsFrom(TH1F::Class())) continue;
        //2D input histograms where Y is the variable tf will depend on and Y slice
        std::map<std::string, std::shared_ptr<TH1D>> yields1D;
        std::map<std::string, std::shared_ptr<TH2F>> yields;

        //Read histograms
        for(const std::string region : regions){
            yields[region] = RUtil::CloneSmart(RUtil::Get<TH2F>(files[{region, "data"}].get(), varName));
            yields[region]->SetDirectory(0);

            for(const std::string& process : processes){
                if(process == "data") continue;
                else yields[region]->Add(RUtil::Get<TH2F>(files[{region, process}].get(), varName), -1);
            }

            yields1D[region] = std::shared_ptr<TH1D>(yields[region]->ProjectionY());
            yields1D[region]->SetDirectory(0);
        }

        //Calculate tf
        if(tf == nullptr){
            //No binning is given
            if(bins.empty()) tf = CalcTF(yields1D);

            //For given binning
            else{
                for(const std::string& region : regions){
                    yields1D[region].reset(static_cast<TH1D*>(yields1D[region]->Rebin(bins.size() - 1, "", bins.data())));
                    yields1D[region]->SetDirectory(0);
                }

                tf = RUtil::CloneSmart(yields1D.at("B").get());
                tf->Divide(yields1D.at("A").get());
            }

            tf->Write();

            if(onlyTF) return; //Return if only tf should be calculated

            //Final tf binning
            binningY = std::vector<double>(tf->GetNbinsX() + 1, 0.);

            for(unsigned int i = 1; i <= tf->GetNbinsX() + 1; ++i){
                binningY[i - 1] = tf->GetXaxis()->GetBinLowEdge(i);
            }
        }

        //Fill 1D histograms with variable on the x-axis with a sum over y direction of the 2D histograms while applying tf
        std::vector<double> binningX(yields.at("C")->GetNbinsX() + 1, 0.);

        for(unsigned int i = 1; i <= yields.at("C")->GetNbinsX() + 1; ++i){
            binningX[i - 1] = yields.at("C")->GetXaxis()->GetBinLowEdge(i);
        }

        std::string name = StrUtil::Split(yields.at("C")->GetName(), "_VS_").at(0);
        std::shared_ptr<TH1F> outHist = std::make_shared<TH1F>(name.c_str(), name.c_str(), binningX.size() - 1, binningX.data());

        for(const std::string& region : regions){
            yields[region] = RUtil::Rebin2D(yields[region], binningX, binningY);
        }

        for(std::size_t x = 1; x <= yields.at("C")->GetNbinsX(); ++x){
            float binContent = 0.;
            float binError = 0.;

            for(std::size_t y = 1; y <= yields.at("C")->GetNbinsY(); ++y){
            std::cout << tf->GetBinContent(y) << std::endl;
                binContent += yields.at("C")->GetBinContent(x, y)*tf->GetBinContent(y);
               // binError = binContent * (
                  //  std::pow(outHist->GetBinError(x, y)/outHist->GetBinContent(x, y), 2) + 
                 //   std::pow(tf->GetBinError(y)/tf->GetBinContent(y), 2));
            }

            outHist->SetBinContent(x, binContent);
            outHist->SetBinError(x, std::sqrt(binError));
        }

        //Write output
        outFile->cd();
        outHist->Write();

        if(eventCount == nullptr){
            eventCount = std::make_shared<TH1F>("EventCount", "EventCount", 1, 0, 1);
            eventCount->SetBinContent(1, outHist->Integral());
            eventCount->Write();
        }
    }
}
