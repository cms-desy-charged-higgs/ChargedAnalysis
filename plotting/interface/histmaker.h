#ifndef HISTMAKER_H
#define HISTMAKER_H

#include "TFile.h"
#include "TH1F.h"
#include "TChain.h"
#include "TTreeReader.h"
#include "ROOT/TDataFrame.hxx"

#include <string>
#include <vector>

class Histmaker{
    
    typedef ROOT::Experimental::TDF::TResultProxy<TH1F> RTH1F;
    typedef ROOT::Experimental::TDF::TInterface<ROOT::Detail::TDF::TCustomColumnBase> RFilter;

    private:
        TFile* outfile;
        TChain* tree;

        ROOT::Experimental::TDataFrame* frame;

        std::vector<RTH1F> histograms;

    public:
        Histmaker(std::string &process, std::vector<std::string> &filenames);
        void ExtractHists(std::vector<std::string> &parameters, const std::string &cut = "0 == 0", const std::string weight = "1", const bool &useLumiXsec = true);
        void Write();
};

#endif
