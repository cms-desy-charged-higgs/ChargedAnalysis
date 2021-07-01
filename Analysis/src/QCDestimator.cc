#include <ChargedAnalysis/Analysis/include/QCDestimator.h>

QCDEstimator::QCDEstimator(const std::vector<std::string>& processes, const std::map<std::pair<std::string, std::string>, std::string>& inputFiles) :
    processes(processes),
    inputFiles(inputFiles){}

void QCDEstimator::Estimate(const std::string& outName){
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
    TList* variables = dataFiles["A"]->GetListOfKeys();

    for(TObject* var : *variables){
        std::string varName = var->GetName();
        
        if(varName == "cutflow") continue;

        //Get histograms
        std::map<std::string, std::shared_ptr<TH1F>> dataHists;
        std::map<std::pair<std::string, std::string>, std::shared_ptr<TH1F>> bkgHists;

        for(const std::string region : regions){
            for(const std::string& process : processes){
                if(process == "data") dataHists[region] = RUtil::GetSmart<TH1F>(dataFiles[region].get(), varName);
                else bkgHists[{region, process}] = RUtil::GetSmart<TH1F>(bkgFiles[{region, process}].get(), varName);
            }
        }
       
        //Output histogram with QCD yield, transfer factor and kappa factor
        std::shared_ptr<TH1F> outHist = RUtil::CloneSmart(dataHists["C"].get());
        std::shared_ptr<TH1F> tfHist = RUtil::CloneSmart(dataHists["C"].get());
        std::shared_ptr<TH1F> kappaHist = RUtil::CloneSmart(dataHists["C"].get());
        outHist->SetDirectory(0);
        tfHist->SetDirectory(0);
        tfHist->SetName(StrUtil::Merge(outHist->GetName(), "_tf").c_str());
        kappaHist->SetDirectory(0);
        kappaHist->SetName(StrUtil::Merge(outHist->GetName(), "_kappa").c_str());
            
        for(int i = 0; i <= outHist->GetNbinsX(); ++i){
            std::map<std::string, float> yields;
            std::map<std::string, float> errCubed;

            //Initialize with data
            for(const std::string region : regions){
                yields[region] = dataHists[region]->GetBinContent(i);
                errCubed[region] = std::pow(dataHists[region]->GetBinError(i), 2);
            }

            //Substract MC background
            for(const std::string region : regions){
                for(const std::string process : processes){
                    if(process == "data") continue;

                    yields[region] -= bkgHists[{region, process}]->GetBinContent(i);
                    errCubed[region] += std::pow(bkgHists[{region, process}]->GetBinError(i), 2);
                }
            }
            
            //Calculate kappa, transfer factors and error
            float tf = yields["B"]/yields["A"];
            float tfErr = std::sqrt(tf*(errCubed["A"]/std::pow(yields["A"], 2) + errCubed["B"]/std::pow(yields["B"], 2)));

            if(std::isnan(tf) or std::isinf(tf) or tf <= 0){
                tf = 0;
                tfErr = 0;
            }

            float kappa = (yields["E"]/yields["F"])/(yields["G"]/yields["H"]);
            float kappaErr = std::sqrt(kappa*(errCubed["E"]/std::pow(yields["E"], 2) + errCubed["F"]/std::pow(yields["F"], 2) +
                                      errCubed["G"]/std::pow(yields["G"], 2) + errCubed["H"]/std::pow(yields["H"], 2)));

            if(std::isnan(kappa) or std::isinf(kappa) or kappa <= 0 or kappa > 2){
                kappa = 1;
                kappaErr = 0;
            }

            if(yields["C"] <= 0) yields["C"] = 0;

            //Calculate yield and fill hist
            yields["D"] = kappa*tf*yields["C"];
            errCubed["D"] = yields["D"]*(std::pow(tf, 2)/std::pow(tfErr, 2) + std::pow(kappaErr, 2)/std::pow(kappa, 2) + 
                             errCubed["C"]/std::pow(yields["C"], 2));


            outHist->SetBinContent(i, yields["D"]);
            outHist->SetBinError(i, yields["D"] != 0  ? std::sqrt(errCubed["D"]) : 0);

            tfHist->SetBinContent(i, tf);
            tfHist->SetBinError(i, tf != 0  ? tfErr : 0);

            kappaHist->SetBinContent(i, kappa);
            kappaHist->SetBinError(i, kappa != 0  ? kappaErr : 0);
        }

        outFile->cd();
        outHist->Write();
        tfHist->Write();
        kappaHist->Write();
    }
}
