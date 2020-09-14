#include <string>

#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/stringutil.h>

int main(int argc, char *argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::string inputFile = parser.GetValue<std::string>("input-file");
    std::string dCachePath = parser.GetValue<std::string>("dcache-path");
    std::string relativePath = parser.GetValue<std::string>("relative-path");
    
    std::string gfalPrefix = "srm://dcache-se-cms.desy.de:8443/srm/managerv2?SFN=";
    std::string fileName = StrUtil::Split(inputFile, "/").back();

    std::string totalPath = StrUtil::Merge(gfalPrefix, dCachePath, relativePath, "/");
    std::string outFile = StrUtil::Merge(dCachePath, relativePath, "/", fileName);

    std::system(("gfal-mkdir -p " + totalPath).c_str());
    std::system(StrUtil::Merge("gfal-copy -f ", inputFile, " ", totalPath, fileName).c_str());
    std::system(("rm -fv " + inputFile).c_str());
    std::system(StrUtil::Merge("ln -sv ", outFile, " ", inputFile).c_str());
}
