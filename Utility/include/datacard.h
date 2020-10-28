#ifndef DATACARD_H
#define DATACARD_H

#include <vector>
#include <string>
#include <utility>
#include <map>
#include <fstream>
#include <iomanip>

#include <TFile.h>
#include <TH1F.h>

#include <ChargedAnalysis/Utility/include/utils.h>
#include <ChargedAnalysis/Utility/include/stringutil.h>

class Datacard {
    private:
        std::vector<std::string> backgrounds;
        std::string signal;
        std::string data;

        std::string channel;

        std::string outDir;
        bool useAsimov;

        std::vector<std::string> systematics;

        std::map<std::string, float> rates;  //Key : process
        //std::map<std::pair<std::string, std::string>, float> normSys;  //Key : {syst name, process}
        //std::map<std::pair<std::string, std::string> shapeSys; //Key : {syst name, process}
        
        

    public:
        Datacard();
        Datacard(const std::vector<std::string>& backgrounds, const std::string& signal, const std::string& data, const std::string& channel, const std::string& outDir, const bool& useAsimov, const std::vector<std::string>& systematics);
        void GetHists(const std::string& histDir, const std::string& discriminant);
        void Write();
};

#endif
