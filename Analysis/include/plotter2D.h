#ifndef PLOTTER2D_H
#define PLOTTER2D_H

#include <TCanvas.h>
#include <TFile.h>
#include <TH2F.h>

#include <ChargedAnalysis/Utility/include/plotutil.h>
#include <ChargedAnalysis/Utility/include/rootutil.h>
#include <ChargedAnalysis/Utility/include/stringutil.h>

class Plotter2D {
    private:
        std::map<std::string, std::vector<std::shared_ptr<TH2F>>> background, signal;
        std::map<std::string, std::shared_ptr<TH2F>> data, bkgSum, systUp, systDown;

        std::vector<std::string> parameters, systematics;

        std::string channel, era;
        std::vector<std::string> bkgProcesses, sigProcesses;
        std::map<std::string, std::vector<std::string>> bkgFiles, sigFiles;
        std::string dataProcess, dataFile;

    public:
        Plotter2D();
        Plotter2D(const std::string& channel, const std::string& era, const std::vector<std::string>& bkgProcesses, const std::map<std::string, std::vector<std::string>>& bkgFiles, const std::vector<std::string>& sigProcesses, const std::map<std::string, std::vector<std::string>>& sigFiles, const std::string& data, const std::string& dataFile, const std::vector<std::string> systematics);
        void ConfigureHists();
        void Draw(std::vector<std::string> &outdirs);
        
};

#endif
