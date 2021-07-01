#ifndef QCD
#define QCD

#include <map>
#include <vector>
#include <string>

#include <TFile.h>
#include <TH1F.h>
#include <TList.h>

#include <ChargedAnalysis/Utility/include/rootutil.h>
#include <ChargedAnalysis/Utility/include/vectorutil.h>

class QCDEstimator{
    private:
        std::vector<std::string> processes;
        std::map<std::pair<std::string, std::string>, std::string> inputFiles;

    public:
        QCDEstimator(const std::vector<std::string>& processes, const std::map<std::pair<std::string, std::string>, std::string>& inputFiles);
        void Estimate(const std::string& outName);
};

#endif
