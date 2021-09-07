#ifndef QCD
#define QCD

#include <map>
#include <vector>
#include <string>

#include <TFile.h>
#include <TH1F.h>
#include <TH1D.h>
#include <TH2.h>
#include <TH2F.h>

#include <ChargedAnalysis/Utility/include/rootutil.h>
#include <ChargedAnalysis/Utility/include/vectorutil.h>

class QCDEstimator{
    private:
        std::string inputTF;
        std::vector<std::string> processes, regions;
        std::map<std::pair<std::string, std::string>, std::string> inputFiles;

        std::shared_ptr<TH1D> CalcTF(std::map<std::string, std::shared_ptr<TH1D>>& yields1D);

    public:
        QCDEstimator(const std::vector<std::string>& processes, const std::vector<std::string>& regions, const std::map<std::pair<std::string, std::string>, std::string>& inputFiles, const std::string& inputTF = "");
        void Estimate(const std::string& outName, const std::vector<double>& bins, const bool& onlyTF = false);
};

#endif
