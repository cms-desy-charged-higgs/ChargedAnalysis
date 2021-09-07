#include <ChargedAnalysis/Analysis/include/treeappender.h>
#include <ChargedAnalysis/Utility/include/parser.h>

#include <ChargedAnalysis/Utility/include/rootutil.h>

int main(int argc, char* argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::string fileName = parser.GetValue<std::string>("out-file");
    std::string treeName = parser.GetValue<std::string>("tree-name");
    int era = parser.GetValue<int>("era");
    std::vector<std::string> functions = parser.GetVector<std::string>("functions");

    std::string dir = fileName.substr(0, StrUtil::Find(fileName, "/").back());
    std::string tmpFile = "tmp.root"; //dir + "/tmp.root";

    TreeAppender appender(fileName, treeName, era, functions);
    appender.Append(tmpFile, parser);

   // if(RUtil::Open(tmpFile)) std::system(("mv -vf " + tmpFile + " " + fileName).c_str());
}

