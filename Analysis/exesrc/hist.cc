#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/backtracer.h>
#include <ChargedAnalysis/Analysis/include/histmaker.h>

int main(int argc, char* argv[]){
    //Class for backtrace
    Backtracer trace;

    //Parser arguments
    Parser parser(argc, argv);

    std::string fileName = parser.GetValue<std::string>("filename");
    std::vector<std::string> parameters = parser.GetVector<std::string>("parameters");
    std::vector<std::string> cuts = parser.GetVector<std::string>("cuts", {});
    std::string outDir = parser.GetValue<std::string>("out-dir");
    std::string outFile = parser.GetValue<std::string>("out-file");
    std::string channel = parser.GetValue<std::string>("channel");
    std::string bkgYieldFac = parser.GetValue<std::string>("bkg-yield-factor", "");
    std::vector<std::string> bkgYieldFacSyst = parser.GetVector<std::string>("bkg-yield-factor-syst", {});
    std::string bkgType = parser.GetValue<std::string>("bkg-type", "");
    std::vector<std::string> scaleSysts = parser.GetVector<std::string>("scale-systs", {""});
    std::vector<std::string> systDirs = parser.GetVector<std::string>("syst-dirs", {});
    int era = parser.GetValue<int>("era", 2017);
    int eventStart = parser.GetValue<int>("event-start");
    int eventEnd = parser.GetValue<int>("event-end");

    //Create treereader instance
    HistMaker h(parameters, cuts, outDir, outFile, channel, systDirs, scaleSysts, era);
    h.Produce(fileName, eventStart, eventEnd, bkgYieldFac, bkgType, bkgYieldFacSyst);
}
