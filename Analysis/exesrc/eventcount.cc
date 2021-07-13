#include <string>
#include <vector>
#include <map>

#include <TFile.h>
#include <TH1F.h>

#include <ChargedAnalysis/Utility/include/rootutil.h>
#include <ChargedAnalysis/Utility/include/vectorutil.h>
#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/csv.h>

bool sortDecreasing(const float& a, const float& b){ 
    return a > b; 
} 

int main(int argc, char *argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::string outDir = parser.GetValue<std::string>("out-dir");
    std::vector<std::string> processes = parser.GetVector<std::string>("processes");
    std::vector<std::string> files = parser.GetVector<std::string>("files");

    CSV eventCounts(outDir + "/eventYields.csv", "w+", {"Process", "EventYield", "EventCount"}, "\t");
    std::vector<float> eventYields(processes.size(), 1.);
    std::vector<float> eventCount(processes.size(), 1.);

    for(int i = 0; i < processes.size(); ++i){
        std::shared_ptr<TFile> f = RUtil::Open(files.at(i));
        eventYields[i] = RUtil::Get<TH1F>(f.get(), "EventCount")->Integral();
        eventCount[i] = RUtil::Get<TH1F>(f.get(), "EventCount")->GetEntries(); 
    }

    std::vector<int> sortedIdx = VUtil::SortedIndices(eventYields, &sortDecreasing);

    for(int i = 0; i < processes.size(); ++i){
        eventCounts.WriteRow(processes[sortedIdx[i]], eventYields[sortedIdx[i]], eventCount[sortedIdx[i]]);
    }
}
