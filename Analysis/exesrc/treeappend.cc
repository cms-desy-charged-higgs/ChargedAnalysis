#include <ChargedAnalysis/Analysis/include/treeappender.h>
#include <ChargedAnalysis/Utility/include/parser.h>

#include <torch/torch.h>

int main(int argc, char* argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::string oldFile = parser.GetValue<std::string>("old-file");
    std::string oldTree = parser.GetValue<std::string>("old-tree");
    std::string newFile = parser.GetValue<std::string>("new-file");
    std::string dCache = parser.GetValue<std::string>("dCache");
    std::vector<std::string> branchNames = parser.GetVector<std::string>("branch-names");

    TreeAppender appender(oldFile, oldTree, newFile, branchNames, dCache);
    appender.Append();
}

