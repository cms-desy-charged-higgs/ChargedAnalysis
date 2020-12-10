#include <string>
#include <vector>

#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/stringutil.h>
#include <ChargedAnalysis/Analysis/include/bkgestimator.h>

int main(int argc, char *argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::vector<std::string> processes = parser.GetVector<std::string>("processes");
    std::string parameter = parser.GetValue<std::string>("parameter");
    std::string outDir = parser.GetValue<std::string>("out-dir");

    BackgroundEstimator estimator(parameter, processes);
    
    for(const std::string& process : processes){
        std::vector<std::string> bkgFiles = parser.GetVector<std::string>(StrUtil::Merge(process, "-bkg-files"));
        std::string dataFile = parser.GetValue<std::string>(StrUtil::Merge(process, "-data-file"));
    
        estimator.AddFiles(process, bkgFiles, dataFile);
    }

    estimator.Estimate(outDir);
}
