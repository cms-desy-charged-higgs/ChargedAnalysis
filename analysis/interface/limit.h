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

        std::vector<std::string> masses;
        std::vector<std::string> channels;
        std::vector<std::string> bkgProc;
        std::vector<std::string> sigProc;

        void SetSyst();

    public:
        Limit();
        Limit(std::vector<std::string> &masses, std::vector<std::string> &channels, std::vector<std::string> &bkgProc);
        void WriteDatacard();
};

#endif
