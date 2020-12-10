#include <ChargedAnalysis/Analysis/include/bkgestimator.h>

BackgroundEstimator::BackgroundEstimator(const std::string& histName, const std::vector<std::string>& processes):
    histName(histName),
    processes(processes){}

void BackgroundEstimator::AddFiles(const std::string& process, const std::vector<std::string>& bkgFiles, const std::string& dataFile){
    for(int i = 0; i < bkgFiles.size(); ++i){
        std::shared_ptr<TFile> bkgFile = RUtil::Open(bkgFiles.at(i));
        
        if(i < processes.size()){
            bkgs[{process, processes[i]}] = RUtil::GetSmart<TH1F>(bkgFile.get(), histName);
            bkgs[{process, processes[i]}]->SetDirectory(0);
        }
        
        else{
            if(!bkgs.count({process, "Misc"})){
                bkgs[{process, "Misc"}] = RUtil::GetSmart<TH1F>(bkgFile.get(), histName);
                bkgs[{process, "Misc"}]->SetDirectory(0);
            }
                        
            else{
                bkgs[{process, "Misc"}]->Add(RUtil::Get<TH1F>(bkgFile.get(), histName));
            }
        }
    }
    
    std::shared_ptr<TFile> dataF = RUtil::Open(dataFile);
    data[process] = RUtil::GetSmart<TH1F>(dataF.get(), histName);
    data[process]->SetDirectory(0);
}

void BackgroundEstimator::Estimate(const std::string& outDir){
    Eigen::MatrixXd bkgYields(processes.size(), processes.size());  
    Eigen::VectorXd dataYields(processes.size());
    Eigen::VectorXd scaleFactors;
    
    for(int i = 0; i < processes.size(); ++i){
        for(int j = 0; j < processes.size(); ++j){
            bkgYields(i, j) = bkgs[{processes[i], processes[j]}]->Integral();
        }
        
        dataYields(i) = data[processes[i]]->Integral();
    }
    
    scaleFactors = bkgYields.inverse() * dataYields;

    CSV scaleFile(outDir + "/scaleFactor.csv", "w+", {"Process", "Factor"}, "\t");
    
    for(int i = 0; i < processes.size(); ++i){
        scaleFile.WriteRow(processes[i], scaleFactors[i]);
    }  
}

