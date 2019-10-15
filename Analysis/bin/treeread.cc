#include <ChargedAnalysis/Analysis/include/utils.h>
#include <ChargedAnalysis/Analysis/include/treereader.h>

int main(int argc, char* argv[]){
    //Extract informations of command line
    std::string process = std::string(argv[1]);
    std::vector<std::string> xParameters = Utils::SplitString(std::string(argv[2]), " ");
    std::vector<std::string> yParameters = Utils::SplitString(std::string(argv[3]), " ");
    std::vector<std::string> cutStrings = Utils::SplitString(std::string(argv[4]), " ");
    std::string outname = std::string(argv[5]);
    std::string channel = std::string(argv[6]);
    std::string saveMode = std::string(argv[7]);
    std::string fileName = argv[8];
    std::vector<std::string> eventYield = Utils::SplitString(std::string(argv[9]), " ");

    //Create treereader instance
    TreeReader reader(process, xParameters, yParameters, cutStrings, outname, channel, saveMode);
    reader.EventLoop(fileName, std::stoi(eventYield[0]), std::stoi(eventYield[1]));
}
