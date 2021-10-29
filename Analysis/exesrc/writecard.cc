#include <vector>
#include <string>
#include <map>

#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/stringutil.h>
#include <ChargedAnalysis/Utility/include/datacard.h>

int main(int argc, char* argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::vector<std::string> bkgProcesses = parser.GetVector("bkg-processes");
    std::string sigProcess = parser.GetValue("sig-process"); 
    std::string data = parser.GetValue("data");

    std::string discriminant = parser.GetValue("discriminant");
    std::string outDir = parser.GetValue("out-dir"); 
    std::string channel = parser.GetValue("channel");
    std::string era = parser.GetValue("era");

    std::vector<std::string> regions = parser.GetVector("region-names", {"SR"});
    std::vector<std::string> systematics = parser.GetVector("systematics", {"Nominal"});

    std::map<std::string, std::map<std::string, std::vector<std::string>>> bkgFiles;
    std::map<std::string, std::map<std::string, std::string>> sigFiles;
    std::map<std::string, std::string> dataFiles;

    for(const std::string& syst : systematics){
        for(const std::string& shift : {"Up", "Down"}){
            for(const std::string& region : regions){
                if(syst == "Nominal" and shift == "Down") continue;

                std::string systName = syst != "Nominal" ? StrUtil::Merge(syst, shift) : "Nominal";

                if(syst == "Nominal") dataFiles[region] = parser.GetValue("data-file-" + region);

                bkgFiles[region][systName] = parser.GetVector(syst != "Nominal" ? StrUtil::Join("-", "bkg-files", region, systName) : "bkg-files-" + region);
                sigFiles[region][systName] = parser.GetValue(syst != "Nominal" ? StrUtil::Join("-", "sig-files", region, systName) : "sig-files-" + region);
            }
        }
    }

    //Create datcard
    Datacard datacard(outDir, channel, era, bkgProcesses, bkgFiles, sigProcess, sigFiles, data, dataFiles, systematics, regions);
    datacard.GetHists(discriminant);
    datacard.Write();
}
