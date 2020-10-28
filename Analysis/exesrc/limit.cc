#include <string>
#include <vector>
#include <unistd.h>

#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/stringutil.h>

int main(int argc, char* argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::string outDir = parser.GetValue<std::string>("out-dir");
    std::string cardDir = parser.GetValue<std::string>("card-dir");
    
    std::string tmp = std::to_string(getppid());

    std::system(StrUtil::Merge("combine ", "-n ", tmp, " ", cardDir, "/datacard.txt").c_str());
    std::system(StrUtil::Merge("mv ", "higgsCombine", tmp, "*.root ", outDir, "/limit.root").c_str());
}
