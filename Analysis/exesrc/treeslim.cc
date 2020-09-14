#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Analysis/include/treeslimmer.h>

int main(int argc, char* argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::string inName = parser.GetValue<std::string>("input-name");
    std::string inChannel = parser.GetValue<std::string>("input-channel");
    std::string outName = parser.GetValue<std::string>("out-name");
    std::string outChannel = parser.GetValue<std::string>("out-channel");
    std::vector<std::string> cuts = parser.GetVector<std::string>("cuts");
    int start = parser.GetValue<int>("event-start");
    int end = parser.GetValue<int>("event-end");

    TreeSlimmer slimmer(inName, inChannel);    
    slimmer.DoSlim(outName, outChannel, cuts, start, end);
}
