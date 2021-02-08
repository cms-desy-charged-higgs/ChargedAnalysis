#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Analysis/include/treeslimmer.h>

int main(int argc, char* argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::string inName = parser.GetValue<std::string>("input-file");
    std::string inChannel = parser.GetValue<std::string>("input-channel");
    std::string outFile = parser.GetValue<std::string>("out-file");
    std::string outChannel = parser.GetValue<std::string>("out-channel");
    std::vector<std::string> cuts = parser.GetVector<std::string>("cuts");
    int start = parser.GetValue<int>("event-start");
    int end = parser.GetValue<int>("event-end");

    //Create output directory if needed
    if(!StrUtil::Find(outFile, "/").empty()){
        std::filesystem::create_directories(outFile.substr(0, StrUtil::Find(outFile, "/").back()));
    }
    
    TreeSlimmer slimmer(inName, inChannel);    
    slimmer.DoSlim(outFile, outChannel, cuts, start, end);
}
