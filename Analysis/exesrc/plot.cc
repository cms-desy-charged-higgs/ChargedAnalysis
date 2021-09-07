#include <string>
#include <vector>

#include <ChargedAnalysis/Utility/include/backtracer.h>
#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/stringutil.h>
#include <ChargedAnalysis/Analysis/include/plotter1D.h>
#include <ChargedAnalysis/Analysis/include/plotter2D.h>

int main(int argc, char *argv[]){
    //Class for backtrace
    //Backtracer trace;

    //Parser arguments
    Parser parser(argc, argv);

    std::string era = parser.GetValue("era", "2017");
    std::string channel = parser.GetValue("channel");
    std::vector<std::string> bkgProcesses = parser.GetVector("bkg-processes", {""});
    std::vector<std::string> sigProcesses = parser.GetVector("sig-processes", {""});
    std::string data = parser.GetValue("data", "");
    std::string dataFile = parser.GetValue("data-file", "");
    std::vector<std::string> outDirs = parser.GetVector("out-dirs");
    std::vector<std::string> systematics = parser.GetVector("systematics", {"Nominal"});
    bool plot1D = parser.GetValue<bool>("plot-1D");
    bool plot2D = parser.GetValue<bool>("plot-2D");

    std::map<std::string, std::vector<std::string>> bkgFiles;
    std::map<std::string, std::vector<std::string>> sigFiles;

    for(const std::string& syst : systematics){
        for(const std::string& shift : {"Up", "Down"}){
            if(syst == "Nominal" and shift == "Down") continue;

            std::string systName = syst != "Nominal" ? StrUtil::Merge(syst, shift) : "Nominal";

            if(!bkgProcesses.empty()) bkgFiles[systName] = parser.GetVector(syst != "Nominal" ? StrUtil::Join("-", "bkg-files", systName) : "bkg-files");
            if(!sigProcesses.empty()) sigFiles[systName] = parser.GetVector(syst != "Nominal" ? StrUtil::Join("-", "sig-files", systName) : "sig-files");
        }
    }

    //Call and run plotter class
    if(plot1D){
        Plotter1D plotter1D(channel, era, bkgProcesses, bkgFiles, sigProcesses, sigFiles, data, dataFile, systematics);
        plotter1D.ConfigureHists();
        plotter1D.Draw(outDirs);
    }

    if(plot2D){
        Plotter2D plotter2D(channel, era, bkgProcesses, bkgFiles, sigProcesses, sigFiles, data, dataFile, systematics);
        plotter2D.ConfigureHists();
        plotter2D.Draw(outDirs);
    }
}
