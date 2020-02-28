#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Analysis/include/treereader.h>

int main(int argc, char* argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::vector<std::string> parameters = parser.GetVector<std::string>("parameters");
    std::vector<std::string> cuts = parser.GetVector<std::string>("cuts");
    std::string outName = parser.GetValue<std::string>("out-name");
    std::string channel = parser.GetValue<std::string>("channel");
    std::string fileName = parser.GetValue<std::string>("filename");
    std::string cleanJet = parser.GetValue<std::string>("clean-jet");
    std::vector<int> eventYield = parser.GetVector<int>("event-yield");

    //Create treereader instance
    TreeReader reader(parameters, cuts, outName, channel);
    reader.EventLoop(fileName, eventYield[0], eventYield[1], cleanJet);
}
