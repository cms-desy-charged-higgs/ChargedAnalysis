#include <ChargedAnalysis/Analysis/include/treeappender.h>
#include <ChargedAnalysis/Utility/include/parser.h>

#include <ChargedAnalysis/Utility/include/rootutil.h>

int main(int argc, char* argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::string inFile = parser.GetValue<std::string>("in-file");
    std::string outFile = parser.GetValue<std::string>("out-file");
    std::string treeName = parser.GetValue<std::string>("tree-name");
    int era = parser.GetValue<int>("era");
    int eventStart = parser.GetValue<int>("event-start");
    int eventEnd = parser.GetValue<int>("event-end");
    std::vector<std::string> functions = parser.GetVector<std::string>("functions");

    std::string dir = outFile.substr(0, StrUtil::Find(outFile, "/").back());
    std::string tmpFile = dir + "/tmp.root";

    TreeAppender appender(inFile, treeName, era, functions);
    appender.Append(tmpFile, eventStart, eventEnd, parser);

    if(RUtil::Open(tmpFile)) std::system(("mv -vf " + tmpFile + " " + outFile).c_str());
}

