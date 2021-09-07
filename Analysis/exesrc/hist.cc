#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/backtracer.h>
#include <ChargedAnalysis/Analysis/include/histmaker.h>

int main(int argc, char* argv[]){
    //Class for backtrace
  //  Backtracer trace;

    //Parser arguments
    Parser parser(argc, argv);

    std::string fileName = parser.GetValue<std::string>("filename");
    std::string channel = parser.GetValue<std::string>("channel");
    std::string outFile = parser.GetValue<std::string>("out-file");

    int era = parser.GetValue<int>("era", 2017);
    int eventStart = parser.GetValue<int>("event-start");
    int eventEnd = parser.GetValue<int>("event-end");

    std::vector<std::string> parameters = parser.GetVector("parameters");
    std::vector<std::string> regions = parser.GetVector("regions");
    if(regions.size() == 0) regions = {""};
    std::vector<std::string> scaleSysts = parser.GetVector("scale-systs");

    std::string bkgYieldFac = parser.GetValue("bkg-yield-factor", "");
    std::vector<std::string> bkgYieldFacSyst = parser.GetVector("bkg-yield-factor-syst", {});
    std::string bkgType = parser.GetValue("bkg-type", "");

    std::string fakeRate = parser.GetValue("fake-rate", "");
    std::string promptRate = parser.GetValue("prompt-rate", "");

    std::map<std::string, std::string> outDir;
    std::map<std::string, std::vector<std::string>> cuts, systDirs;

    for(const std::string& region : regions){
        outDir[region] = parser.GetValue(region + "-out-dir");

        cuts[region] = parser.GetVector(region + "-cuts");
        systDirs[region] = parser.GetVector(region + "-syst-dirs");
    }

    //Create treereader instance
    HistMaker h(parameters, regions, cuts, outDir, outFile, channel, systDirs, scaleSysts, fakeRate, promptRate, era);
    h.Produce(fileName, eventStart, eventEnd, bkgYieldFac, bkgType, bkgYieldFacSyst);
}
