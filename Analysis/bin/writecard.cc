#include <ChargedAnalysis/Analysis/include/utils.h>
#include <ChargedAnalysis/Analysis/include/datacard.h>

int main(int argc, char* argv[]){
    //Extract informations of command line
    std::vector<std::string> backgrounds = Utils::SplitString(std::string(argv[1]), " ");
    std::string signal = std::string(argv[2]);
    std::string data = std::string(argv[3]);
    std::string channel = std::string(argv[4]);
    std::string outDir = std::string(argv[5]);
    bool useAsimov = std::string(argv[6]) == "true";
    std::string histDir = std::string(argv[7]);
    std::string discriminant = std::string(argv[8]);

    //Create datcard
    Datacard datacard(backgrounds, signal, data, channel, outDir, useAsimov);
    datacard.GetHists(histDir, discriminant);
    datacard.Write();
}
