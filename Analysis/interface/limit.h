#ifndef LIMIT_H
#define LIMIT_H

#include <vector>
#include <string>

#include <TFile.h>

#include <CombineHarvester/CombineTools/interface/CombineHarvester.h>
#include <CombineHarvester/CombineTools/interface/Systematics.h>

class Limit {
    private:
        ch::CombineHarvester cb;
        ch::Categories bins;

        std::string mass;
        std::vector<std::string> channels;
        std::vector<std::string> bkgProc;
        std::string outDir;
        std::vector<std::string> sigProc;

        std::map<std::string, std::string> chanToDir;

        void SetSyst();

    public:
        Limit();
        Limit(std::string &mass, std::vector<std::string> &channels, std::vector<std::string> &bkgProc, std::string &outDir);
        void WriteDatacard(std::string &histDir, std::string &parameter);
        void CalcLimit();
};

#endif
