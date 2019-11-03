#include <ChargedAnalysis/Analysis/include/treeappender.h>
#include <ChargedAnalysis/Analysis/include/utils.h>

#include <torch/torch.h>

int main(int argc, char* argv[]){
    //Extract informations of command line
    std::string oldFile = std::string(argv[1]);
    std::string oldTree = std::string(argv[2]);
    std::string newFile = std::string(argv[3]);
    std::vector<std::string> branchNames = Utils::SplitString(std::string(argv[4]), " ");
    std::string entryStart = std::string(argv[5]);
    std::string entryEnd = std::string(argv[6]);

    at::set_num_interop_threads(1);
    at::set_num_threads(1);

    TreeAppender appender(oldFile, oldTree, newFile, branchNames, std::stoi(entryStart), std::stoi(entryEnd));
    appender.Append();
}

