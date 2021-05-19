#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Analysis/include/histmaker.h>

int main(int argc, char* argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::string fileName = parser.GetValue<std::string>("filename");
    std::vector<std::string> parameters = parser.GetVector<std::string>("parameters");
    std::vector<std::string> cuts = parser.GetVector<std::string>("cuts", {});
    std::string outDir = parser.GetValue<std::string>("out-dir");
    std::string outFile = parser.GetValue<std::string>("out-file");
    std::string channel = parser.GetValue<std::string>("channel");
    std::vector<std::string> scaleDirs = parser.GetVector<std::string>("syst-dirs", {});
    std::vector<std::string> scaleSysts = parser.GetVector<std::string>("scale-systs", {""});
    std::string scaleFactors = parser.GetValue<std::string>("scale-factors", "");
    int era = parser.GetValue<int>("era", 2017);

    //Create treereader instance
    HistMaker h(parameters, cuts, outDir, outFile, channel, scaleDirs, scaleSysts, scaleFactors, era);
    h.Produce(fileName);
}
