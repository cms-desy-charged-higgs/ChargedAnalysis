#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Analysis/include/treereader.h>

int main(int argc, char* argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::string process = parser.GetValue<std::string>("process");
    std::vector<std::string> xParameters = parser.GetVector<std::string>("x-parameters");
    std::vector<std::string> yParameters = parser.GetVector<std::string>("y-parameters");
    std::vector<std::string> cuts = parser.GetVector<std::string>("cuts");
    std::string outName = parser.GetValue<std::string>("out-name");
    std::string channel = parser.GetValue<std::string>("channel");
    std::string saveMode = parser.GetValue<std::string>("save-mode");
    std::string fileName = parser.GetValue<std::string>("filename");
    std::vector<int> eventYield = parser.GetVector<int>("event-yield");

    //Create treereader instance
    TreeReader reader(process, xParameters, yParameters, cuts, outName, channel, saveMode);
    reader.EventLoop(fileName, eventYield[0], eventYield[1]);
}
