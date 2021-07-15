#include <ChargedAnalysis/Analysis/include/QCDestimator.h>

QCDEstimator::QCDEstimator(const std::vector<std::string>& processes, const std::map<std::pair<std::string, std::string>, std::string>& inputFiles) :
    processes(processes),
    inputFiles(inputFiles){}

void QCDEstimator::Estimate(const std::string& outName, const std::vector<double>& bins){
    std::vector<std::string> regions = {"A", "B", "C", "E", "F", "G", "H"}; 

    //Open data/background files
    std::map<std::string, std::shared_ptr<TFile>> dataFiles;
    std::map<std::pair<std::string, std::string>, std::shared_ptr<TFile>> bkgFiles;

    for(const std::string region : regions){
        for(const std::string& process : processes){
            if(process == "data") dataFiles[region] = RUtil::Open(inputFiles.at({region, "data"}));
            else bkgFiles[{region, process}] = RUtil::Open(inputFiles.at({region, process}));
        }
    }

    //Open output file
    std::shared_ptr<TFile> outFile = std::make_shared<TFile>(outName.c_str(), "RECREATE");

    //1D histograms for tf, kappa
    std::shared_ptr<TH1D> tf, kappa;

    //Y slice of 2D input histograms where Y is the variable kappa/tf will depend on
    std::map<std::string, std::shared_ptr<TH1D>> yieldsInY;
    std::vector<double> binning;

    //List of variables
    for(const std::string varName : RUtil::ListOfContent(dataFiles["A"].get())){
        if(!dataFiles["A"]->Get(varName.c_str())->InheritsFrom(TH2::Class())) continue;

        //Get histograms with variable on interest on x-axis and it dependent variable on the y-axis
        std::map<std::string, std::shared_ptr<TH2F>> dataHists;
        std::map<std::pair<std::string, std::string>, std::shared_ptr<TH2F>> bkgHists;

        for(const std::string region : regions){
            for(const std::string& process : processes){
                if(process == "data") dataHists[region] = RUtil::GetSmart<TH2F>(dataFiles[region].get(), varName);
                else bkgHists[{region, process}] = RUtil::GetSmart<TH2F>(bkgFiles[{region, process}].get(), varName);
            }
        }

        //Calculate yields and for 2D and only y-axis
        std::map<std::string, std::shared_ptr<TH2F>> yields;

        for(const std::string region : regions){
            yields[region] = RUtil::CloneSmart(dataHists[region].get());
        }

        for(const std::string region : regions){
            for(const std::string process : processes){
                if(process == "data") continue;

                yields[region]->Add(bkgHists[{region, process}].get(), -1);
            }

            if(tf == nullptr) yieldsInY[region] = RUtil::CloneSmart(yields[region]->ProjectionY());
        }

        //Calculate tf and kappa in depedence of the parameter on y-axis
        if(tf == nullptr){
            //Calculate kappa/tf for initial binning
            tf = RUtil::CloneSmart(yieldsInY["C"].get());
            kappa = RUtil::CloneSmart(yieldsInY["C"].get());

            tf->SetDirectory(0);
            tf->Divide(yieldsInY["B"].get(), yieldsInY["A"].get());

            kappa->SetDirectory(0);
            kappa->Divide(yieldsInY["E"].get(), yieldsInY["F"].get());
            kappa->Multiply(yieldsInY["H"].get());
            kappa->Divide(yieldsInY["G"].get());

            //Get initial binning (n bins with n lower edges + 1 upper edge for last bin)
            if(bins.size() == 0){
                binning = std::vector<double>(tf->GetNbinsX() + 1, 0.);
                
                for(unsigned int i = 1; i <= tf->GetNbinsX() + 1; ++i){
                    binning[i - 1] = tf->GetBinLowEdge(i);
                }

                //Rebinning
                float errMin = 0.2;
                int maxBin = 10;

                while(true){
                    bool finished = false;

                    //Start with last lower edge and end with second lower edge to avoid purging first lower edge
                    for(unsigned int i = binning.size() - 1; i > 1; --i){
                        finished = true;

                        float relErr = std::max(tf->GetBinError(i)/tf->GetBinContent(i), kappa->GetBinError(i)/kappa->GetBinContent(i));

                        //If relative error too big, remove lower edge at position i-1 (to the left on the current lower edge)
                        if(std::abs(relErr) > errMin or tf->GetBinContent(i) <= 0 or kappa->GetBinContent(i) <= 0){
                            binning.erase(binning.begin() + i - 1);

                            //Recalculate tf/kappa with rebinned histogram
                            tf.reset(static_cast<TH1D*>(tf->Rebin(binning.size() - 1, "", binning.data())));
                            tf->Divide(yieldsInY["B"]->Rebin(binning.size() - 1, "B", binning.data()), yieldsInY["A"]->Rebin(binning.size() - 1, "A", binning.data()));

                            kappa.reset(static_cast<TH1D*>(tf->Rebin(binning.size() - 1, "", binning.data())));
                            kappa->Divide(yieldsInY["E"]->Rebin(binning.size() - 1, "E", binning.data()), yieldsInY["F"]->Rebin(binning.size() - 1, "F", binning.data()));
                            kappa->Multiply(yieldsInY["H"]->Rebin(binning.size() - 1, "H", binning.data()));
                            kappa->Divide(yieldsInY["G"]->Rebin(binning.size() - 1, "G", binning.data()));

                            finished = false;
                            break;
                        }
                    }

                    if(binning.size() == 2) throw std::runtime_error("No convergence of kappa/transfer factor! Check purity of your control regions!");

                    if(binning.size() - 1 > maxBin and finished){
                        errMin -= 0.01;
                        continue;
                    }
     
                    if(finished) break;
                }

                std::cout << "Final binning:" << std::endl;
                std::cout << binning << std::endl;
            }

            else{
                tf.reset(static_cast<TH1D*>(tf->Rebin(bins.size() - 1, "", bins.data())));
                tf->Divide(yieldsInY["B"]->Rebin(bins.size() - 1, "B", bins.data()), yieldsInY["A"]->Rebin(bins.size() - 1, "A", bins.data()));

                kappa.reset(static_cast<TH1D*>(tf->Rebin(bins.size() - 1, "", bins.data())));
                kappa->Divide(yieldsInY["E"]->Rebin(bins.size() - 1, "E", bins.data()), yieldsInY["F"]->Rebin(bins.size() - 1, "F", bins.data()));
                kappa->Multiply(yieldsInY["H"]->Rebin(bins.size() - 1, "H", bins.data()));
                kappa->Divide(yieldsInY["G"]->Rebin(bins.size() - 1, "G", bins.data()));


                std::cout << "Used binning:" << std::endl;
                std::cout << bins << std::endl;
            }

            // tf/kappa write to file
            tf->SetDirectory(0);
            tf->SetName("tf");
            tf->SetTitle("tf");
            tf->Write();

            kappa->SetDirectory(0);
            kappa->SetName("kappa");
            kappa->SetTitle("kappa");
            kappa->Write();
        }

        //Fill 1D histograms with variable on the axis with a sum over y direction of the 2D histograms while applying kappa/tf
        std::string outName = StrUtil::Split(yields["C"]->GetName(), "_VS").at(0);
        std::shared_ptr<TH1F> outHist = std::make_shared<TH1F>(outName.c_str(), outName.c_str(), yields["C"]->GetNbinsX(), yields["C"]->GetXaxis()->GetXmin(), yields["C"]->GetXaxis()->GetXmax());

        for(unsigned int i = 1; i <= yields["C"]->GetNbinsX(); ++i){
            float qcdYield = 0;
            float error = 0;

            for(unsigned int j = 1; j <= yields["C"]->GetNbinsY(); ++j){
                if(yields["C"]->GetBinContent(i, j) == 0) continue;
                float yBin = tf->FindBin(yields["C"]->GetYaxis()->GetBinCenter(j));

                float binValue = yields["C"]->GetBinContent(i, j) * tf->GetBinContent(yBin) * kappa->GetBinContent(yBin);
                float binError = std::pow(binValue, 2) * (
                         std::pow(yields["C"]->GetBinError(i, j)/yields["C"]->GetBinContent(i, j), 2) + 
                         std::pow(tf->GetBinError(yBin)/tf->GetBinContent(yBin), 2) + 
                         std::pow(kappa->GetBinError(yBin)/kappa->GetBinContent(yBin), 2));

                qcdYield += binValue;
                error += binError;
            }

            outHist->SetBinContent(i, qcdYield);
            outHist->SetBinError(i, std::sqrt(error));
        }

        //Event count
        if(!outFile->GetListOfKeys()->Contains("EventCount")){
            double error;

            std::shared_ptr<TH1F> eventCount = std::make_shared<TH1F>("EventCount", "EventCount", 1, 0, 1);
            eventCount->SetBinContent(1, outHist->IntegralAndError(1, outHist->GetNbinsX(), error));
            eventCount->SetBinError(1, error);

            eventCount->Write();
        }

        //Write output
        outFile->cd();
        outHist->Write();
    }
}
