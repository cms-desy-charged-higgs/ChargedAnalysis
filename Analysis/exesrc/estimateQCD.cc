#include <string>
#include <vector>

#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/stringutil.h>
#include <ChargedAnalysis/Analysis/include/QCDestimator.h>

int main(int argc, char *argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::vector<std::string> processes = parser.GetVector("processes");
    std::string outFile = parser.GetValue("out-file");
    std::map<std::pair<std::string, std::string>, std::string> inputFiles; //region, process : filename 
    std::vector<std::string> regions = {"A", "B", "C", "E", "F", "G", "H"}; 
    
    for(const std::string region : regions){
        for(const std::string& process : processes){
            inputFiles[{region, process}] = parser.GetValue(StrUtil::Join("-", region, process, "file"));
        }
    }

    QCDEstimator estimator(processes, inputFiles);
    estimator.Estimate(outFile);
}
