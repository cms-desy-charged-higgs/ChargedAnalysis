#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/datacard.h>

int main(int argc, char* argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::vector<std::string> bkgProcesses = parser.GetVector<std::string>("bkg-processes");
    std::string sigProcess = parser.GetValue<std::string>("sig-process"); 
    std::string data = parser.GetValue<std::string>("data");

    std::vector<std::string> bkgFiles = parser.GetVector<std::string>("bkg-files");
    std::vector<std::string> sigFiles = parser.GetVector<std::string>("sig-files");
    std::string dataFile = parser.GetValue<std::string>("data-file");

    std::string discriminant = parser.GetValue<std::string>("discriminant");
    std::string outDir = parser.GetValue<std::string>("out-dir"); 
    std::string channel = parser.GetValue<std::string>("channel"); 
    std::vector<std::string> systematics = parser.GetVector<std::string>("systematics", {""});

    //Create datcard
    Datacard datacard(outDir, channel, bkgProcesses, bkgFiles, sigProcess, sigFiles, data, dataFile, systematics);
    datacard.GetHists(discriminant);
    datacard.Write();
}
