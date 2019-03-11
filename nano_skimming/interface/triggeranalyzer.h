#ifndef TRIGGERANALYZER_H
#define TRIGGERANALYZER_H

#include <ChargedHiggs/nano_skimming/interface/baseanalyzer.h>

#include <numeric>

class TriggerAnalyzer : public BaseAnalyzer {
    private:
        //Trigger strings and vector with values
        std::vector<std::string> triggerPaths;
        std::vector<std::unique_ptr<TTreeReaderValue<bool>>> triggerValues;
        
        //Vector with triger results
        std::vector<int> triggerResults;

    public:
        TriggerAnalyzer(const std::vector<std::string> &triggerPaths);
        void BeginJob(TTreeReader &reader, TTree *tree, bool &isData);
        bool Analyze();
        void EndJob(TFile* file);
};

#endif
