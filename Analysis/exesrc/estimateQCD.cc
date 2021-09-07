#include <string>
#include <vector>

#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/backtracer.h>
#include <ChargedAnalysis/Utility/include/stringutil.h>
#include <ChargedAnalysis/Analysis/include/QCDestimator.h>

int main(int argc, char *argv[]){
    //Class for backtrace
    Backtracer trace;

    //Parser arguments
    Parser parser(argc, argv);

    std::vector<std::string> processes = parser.GetVector("processes");
    std::vector<std::string> regions = parser.GetVector("regions");
    std::vector<double> binning = parser.GetVector<double>("binning", std::initializer_list<double>());
    bool onlyTF = parser.GetValue<bool>("only-tf");
    std::string outFile = parser.GetValue("out-file");

    std::map<std::pair<std::string, std::string>, std::string> inputFiles; //region, process : filename 

    for(const std::string region : regions){
        for(const std::string& process : processes){
            inputFiles[{region, process}] = parser.GetValue(StrUtil::Join("-", region, process, "file"));
        }
    }

    QCDEstimator estimator(processes, regions, inputFiles);
    estimator.Estimate(outFile, binning, onlyTF);
}
