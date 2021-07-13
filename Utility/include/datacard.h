#ifndef DATACARD_H
#define DATACARD_H

#include <vector>
#include <string>
#include <utility>
#include <map>
#include <iostream>
#include <fstream>
#include <iomanip>

#include <TFile.h>
#include <TH1F.h>

#include <ChargedAnalysis/Utility/include/stringutil.h>
#include <ChargedAnalysis/Utility/include/vectorutil.h>
#include <ChargedAnalysis/Utility/include/rootutil.h>

class Datacard {
    private:
        std::string outDir, channel, data, sigProcess, era;
        std::vector<std::string> bkgProcesses, systematics, regions;
        std::map<std::string, std::string> dataFile;
        std::map<std::string, std::map<std::string, std::string>> sigFiles;
        std::map<std::string, std::map<std::string, std::vector<std::string>>> bkgFiles;

        std::map<std::tuple<std::string, std::string>, float> rates;  //region, process
        std::map<std::tuple<std::string, std::string, std::string, std::string>, float> relSys; //region, process, syst, shift

    public:
        Datacard();
        Datacard(const std::string& outDir, const std::string& channel, const std::string era, const std::vector<std::string>& bkgProcesses, const std::map<std::string, std::map<std::string, std::vector<std::string>>>& bkgFiles, const std::string& sigProcess, const std::map<std::string, std::map<std::string, std::string>>& sigFiles, const std::string& data, const std::map<std::string, std::string>& dataFile, const std::vector<std::string>& systematics, const std::vector<std::string>& regions);
        void GetHists(const std::string& discriminant, const bool& blind = true);
        void Write();
};

#endif
