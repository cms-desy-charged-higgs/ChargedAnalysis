#ifndef DATACARD_H
#define DATACARD_H

#include <vector>
#include <string>
#include <utility>
#include <map>
#include <fstream>
#include <iomanip>

#include <TFile.h>
#include <TH1F.h>

#include <ChargedAnalysis/Utility/include/stringutil.h>
#include <ChargedAnalysis/Utility/include/rootutil.h>

class Datacard {
    private:
        std::string outDir, channel, data, dataFile, sigProcess;
        std::vector<std::string> bkgProcesses, systematics;
        std::map<std::string, std::string> sigFiles;
        std::map<std::string, std::vector<std::string>> bkgFiles;

        std::map<std::string, float> rates;  //Key : process

    public:
        Datacard();
        Datacard(const std::string& outDir, const std::string& channel, const std::vector<std::string>& bkgProcesses, const std::map<std::string, std::vector<std::string>>& bkgFiles, const std::string& sigProcess, const std::map<std::string, std::string>& sigFiles, const std::string& data, const std::string& dataFile, const std::vector<std::string>& systematics);
        void GetHists(const std::string& discriminant);
        void Write();
};

#endif
