#include <ChargedAnalysis/Analysis/include/treeappender.h>
#include <ChargedAnalysis/Utility/include/parser.h>

#include <torch/torch.h>

int main(int argc, char* argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::string fileName = parser.GetValue<std::string>("file-name");
    std::string treeName = parser.GetValue<std::string>("tree-name");
    std::string dCache = parser.GetValue<std::string>("dCache", "");
    std::vector<std::string> functions = parser.GetVector<std::string>("functions");

    std::string tmpFile = "tmp_" + std::to_string(getpid()) + ".root";

    TreeAppender appender(fileName, treeName, functions);
    appender.Append(tmpFile);

    std::system(("mv -vf " + tmpFile + " " + fileName).c_str());
    if(dCache != "") Utils::CopyToCache(fileName, dCache);
}

