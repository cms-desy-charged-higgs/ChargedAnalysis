#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/datacard.h>

int main(int argc, char* argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::vector<std::string> backgrounds = parser.GetVector<std::string>("backgrounds");
    std::string signal = parser.GetValue<std::string>("signal"); 
    std::string data = parser.GetValue<std::string>("data"); 
    std::string channel = parser.GetValue<std::string>("channel"); 
    std::string outDir = parser.GetValue<std::string>("out-dir"); 
    bool useAsimov = parser.GetValue<bool>("use-asimov"); 
    std::string histDir = parser.GetValue<std::string>("hist-dir"); 
    std::string discriminant = parser.GetValue<std::string>("discriminant");
    std::vector<std::string> systematics = parser.GetVector<std::string>("systematics", {""});

    //Create datcard
    Datacard datacard(backgrounds, signal, data, channel, outDir, useAsimov, systematics);
    datacard.GetHists(histDir, discriminant);
    datacard.Write();
}
