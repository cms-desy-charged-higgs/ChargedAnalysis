#include <ChargedAnalysis/Analysis/include/treeappender.h>
#include <ChargedAnalysis/Utility/include/parser.h>

#include <torch/torch.h>

int main(int argc, char* argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::string oldFile = parser.GetValue<std::string>("old-file");
    std::string oldTree = parser.GetValue<std::string>("old-tree");
    std::string newFile = parser.GetValue<std::string>("new-file");
    std::vector<std::string> branchNames = parser.GetVector<std::string>("branch-names");
    int entryStart = parser.GetValue<int>("entry-start");
    int entryEnd = parser.GetValue<int>("entry-end");

    at::set_num_interop_threads(1);
    at::set_num_threads(1);

    TreeAppender appender(oldFile, oldTree, newFile, branchNames, entryStart, entryEnd);
    appender.Append();
}

