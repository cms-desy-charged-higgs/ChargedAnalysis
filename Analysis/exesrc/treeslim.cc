#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Analysis/include/treeslimmer.h>

int main(int argc, char* argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::string inName = parser.GetValue<std::string>("input-name");
    std::string inChannel = parser.GetValue<std::string>("input-channel");
    std::string outName = parser.GetValue<std::string>("out-name");
    std::string outChannel = parser.GetValue<std::string>("out-channel");
    std::string dCache = parser.GetValue<std::string>("dCache", "");
    std::vector<std::string> cuts = parser.GetVector<std::string>("cuts");

    std::string tmpFile = "tmp_" + std::to_string(getpid()) + ".root";

    TreeSlimmer slimmer(inName, inChannel);    
    slimmer.DoSlim(tmpFile, outChannel, cuts);

    std::system(("mv -vf " + tmpFile + " " + outName).c_str());
    if(dCache != "") Utils::CopyToCache(outName, dCache);
}
