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

    std::string dataFile = parser.GetValue("data-file");

    std::string discriminant = parser.GetValue("discriminant");
    std::string outDir = parser.GetValue("out-dir"); 
    std::string channel = parser.GetValue("channel"); 
    std::vector<std::string> systematics = parser.GetVector("systematics", {""});

    std::map<std::string, std::vector<std::string>> bkgFiles;
    std::map<std::string, std::string> sigFiles;

    for(const std::string& syst : systematics){
        for(const std::string& shift : {"Up", "Down"}){
            if(syst == "" and shift == "Down") continue;

            std::string systName = syst != "" ? StrUtil::Merge(syst, shift) : "";

            bkgFiles[systName] = parser.GetVector(syst != "" ? StrUtil::Join("-", "bkg-files", systName) : "bkg-files");
            sigFiles[systName] = parser.GetValue(syst != "" ? StrUtil::Join("-", "sig-files", systName) : "sig-files");
        }
    }

    //Create datcard
    Datacard datacard(outDir, channel, bkgProcesses, bkgFiles, sigProcess, sigFiles, data, dataFile, systematics);
    datacard.GetHists(discriminant);
    datacard.Write();
}
