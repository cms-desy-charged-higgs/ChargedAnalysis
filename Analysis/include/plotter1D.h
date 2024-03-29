#ifndef PLOTTER1D_H
#define PLOTTER1D_H

#include <map>
#include <vector>
#include <string>

#include <TFile.h>
#include <TH1F.h>
#include <TGraph.h>
#include <THStack.h>
#include <TLegend.h>
#include <TCanvas.h>
#include <TPad.h>
#include <Rtypes.h>

#include <ChargedAnalysis/Utility/include/plotutil.h>
#include <ChargedAnalysis/Utility/include/rootutil.h>
#include <ChargedAnalysis/Utility/include/stringutil.h>

class Plotter1D {
    private:
        std::map<std::string, std::vector<std::shared_ptr<TH1F>>> background, signal;
        std::map<std::string, std::shared_ptr<TH1F>> data, bkgSum, systUp, systDown;

        std::vector<std::string> parameters, systematics;

        std::string channel, era;
        std::vector<std::string> bkgProcesses, sigProcesses;
        std::map<std::string, std::vector<std::string>> bkgFiles, sigFiles;
        std::string dataProcess, dataFile;


    public:
        Plotter1D();
        Plotter1D(const std::string& channel, const std::string& era, const std::vector<std::string>& bkgProcesses, const std::map<std::string, std::vector<std::string>>& bkgFiles, const std::vector<std::string>& sigProcesses, const std::map<std::string, std::vector<std::string>>& sigFiles, const std::string& data, const std::string& dataFile, const std::vector<std::string> systematics);
        void ConfigureHists();
        void Draw(std::vector<std::string> &outdirs);   
};

#endif
