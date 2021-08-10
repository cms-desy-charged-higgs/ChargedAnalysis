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

    //List of variables
    for(const std::string varName : RUtil::ListOfContent(dataFiles["A"].get())){
        if(dataFiles["A"]->Get(varName.c_str())->InheritsFrom(TH2::Class())) continue;

        //1D histograms for tf, kappa
        std::shared_ptr<TH1F> tf, kappa;

        //Y slice of 2D input histograms where Y is the variable kappa/tf will depend on
        std::map<std::string, std::shared_ptr<TH1F>> yields;
        std::vector<double> binning;

        //Get histograms with variable on interest on x-axis and it dependent variable on the y-axis
        std::map<std::string, std::shared_ptr<TH1F>> dataHists;
        std::map<std::pair<std::string, std::string>, std::shared_ptr<TH1F>> bkgHists;

        for(const std::string region : regions){
            for(const std::string& process : processes){
                if(process == "data") dataHists[region] = RUtil::GetSmart<TH1F>(dataFiles[region].get(), varName);
                else bkgHists[{region, process}] = RUtil::GetSmart<TH1F>(bkgFiles[{region, process}].get(), varName);
            }
        }

        for(const std::string region : regions){
            yields[region] = RUtil::CloneSmart(dataHists[region].get());
        }

        for(const std::string region : regions){
            for(const std::string process : processes){
                if(process == "data") continue;

                yields[region]->Add(bkgHists[{region, process}].get(), -1);
            }
        }

        //Calculate kappa/tf for initial binning
        tf = RUtil::CloneSmart(yields["C"].get());
        tf->SetDirectory(0);
        tf->SetName((varName + "_tf").c_str());
        tf->SetTitle((varName + "_tf").c_str());

        kappa = RUtil::CloneSmart(yields["C"].get());
        kappa->SetDirectory(0);
        kappa->SetName((varName + "_kappa").c_str());
        kappa->SetTitle((varName + "_kappa").c_str());

        for(std::size_t bin = 1; bin <= tf->GetNbinsX(); ++bin){
            float tfValue = 0.;
            float tfError = 0.;

            if(yields["B"]->GetBinContent(bin) > 0 and yields["A"]->GetBinContent(bin) > 0){
                tfValue = yields["B"]->GetBinContent(bin)/yields["A"]->GetBinContent(bin);
                tfError = tfValue * (
                    std::pow(yields["B"]->GetBinError(bin)/yields["B"]->GetBinContent(bin), 2) +
                    std::pow(yields["A"]->GetBinError(bin)/yields["A"]->GetBinContent(bin), 2));
            }

            tf->SetBinContent(bin, tfValue);
            tf->SetBinError(bin, std::sqrt(tfError));

            float kappaValue = 0.;
            float kappaError = 0.;

            if(yields["E"]->GetBinContent(bin) > 0 and yields["F"]->GetBinContent(bin) > 0 and yields["G"]->GetBinContent(bin) > 0 and yields["H"]->GetBinContent(bin) > 0){
                kappaValue = (yields["E"]->GetBinContent(bin)/yields["F"]->GetBinContent(bin))*(yields["H"]->GetBinContent(bin)/yields["G"]->GetBinContent(bin));
                kappaError = kappaValue * (
                    std::pow(yields["E"]->GetBinError(bin)/yields["E"]->GetBinContent(bin), 2) +
                    std::pow(yields["F"]->GetBinError(bin)/yields["F"]->GetBinContent(bin), 2) +
                    std::pow(yields["G"]->GetBinError(bin)/yields["G"]->GetBinContent(bin), 2) +
                    std::pow(yields["H"]->GetBinError(bin)/yields["H"]->GetBinContent(bin), 2));
            }

            kappa->SetBinContent(bin, kappaValue);
            kappa->SetBinError(bin, std::sqrt(kappaError));
        }

        // tf/kappa write to file
        tf->Write();
        kappa->Write();

        //Fill 1D histograms with variable on the axis with a sum over y direction of the 2D histograms while applying kappa/tf
        std::shared_ptr<TH1F> outHist = RUtil::CloneSmart(yields["C"].get());

        for(std::size_t bin = 1; bin <= outHist->GetNbinsX(); ++bin){
            float binContent = 0.;
            float binError = 0.;

            if(outHist->GetBinContent(bin) > 0){
                binContent = outHist->GetBinContent(bin)*tf->GetBinContent(bin)*kappa->GetBinContent(bin);
                binError = binContent * (
                            std::pow(outHist->GetBinError(bin)/outHist->GetBinContent(bin), 2) + 
                            std::pow(tf->GetBinError(bin)/tf->GetBinContent(bin), 2) + 
                            std::pow(kappa->GetBinError(bin)/kappa->GetBinContent(bin), 2));
            }

            outHist->SetBinContent(bin, binContent);
            outHist->SetBinError(bin, std::sqrt(binError));
        }

        //Write output
        outFile->cd();
        outHist->Write();
    }
}
