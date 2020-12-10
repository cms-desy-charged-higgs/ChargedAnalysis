#ifndef BKGESTIMATOR_H
#define BKGESTIMATOR_H

#include <vector>
#include <string>

#include <eigen3/Eigen/Core>
#include <eigen3/Eigen/LU>

#include <TH1F.h>
#include <TFile.h>

#include <ChargedAnalysis/Utility/include/rootutil.h>
#include <ChargedAnalysis/Utility/include/stringutil.h>
#include <ChargedAnalysis/Utility/include/vectorutil.h>
#include <ChargedAnalysis/Utility/include/csv.h>

class BackgroundEstimator{
    private:   
        std::string histName;
        std::vector<std::string> processes;

        std::map<std::string, std::shared_ptr<TH1F>> data;
        std::map<std::string, std::shared_ptr<TH1F>> misc;
        std::map<std::pair<std::string, std::string>, std::shared_ptr<TH1F>> bkgs;    
    
    public:
        BackgroundEstimator(const std::string& histName, const std::vector<std::string>& processes);
        void AddFiles(const std::string& process, const std::vector<std::string>& bkgFiles, const std::string& dataFile);        
        void Estimate(const std::string& outDir);        
};

#endif
