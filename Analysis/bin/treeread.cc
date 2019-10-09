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
    bool saveTree = std::string(argv[7]) == "True" ? true : false;
    bool saveCsv = std::string(argv[8]) == "True" ? true : false;
    std::string fileName = argv[9];
    std::vector<std::string> eventYield = Utils::SplitString(std::string(argv[10]), " ");

    //Create treereader instance
    TreeReader reader(process, xParameters, yParameters, cutStrings, outname, channel, saveTree, saveCsv);
    reader.EventLoop(fileName, std::stoi(eventYield[0]), std::stoi(eventYield[1]));
}
