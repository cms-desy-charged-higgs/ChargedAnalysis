#include "ChargedHiggs/plotting/interface/histmaker.h"

Histmaker::Histmaker(std::string &process, std::vector<std::string> &filenames){
    
    histograms = {};
     
    std::string outname = process + std::string(".root");
    outfile = TFile::Open(outname.c_str(), "RECREATE");

    frame = new ROOT::Experimental::TDataFrame("Events", filenames);
}

void Histmaker::ExtractHists(std::vector<std::string> &parameters, const std::string &cut, const std::string weight, const bool &useLumiXsec) {

    std::string weights = useLumiXsec ? weight + "*xsec*lumi/ngen" : weight;
        
    RFilter filter = frame->Filter(cut).Define("weights", weights);

    for(std::string parameter: parameters){
        RTH1F hist = filter.Histo1D(parameter, "weights");
        hist->SetName(parameter.c_str());        

        histograms.push_back(hist);
    }

}

void Histmaker::Write() {
    for(RTH1F hist: histograms){
        hist->Write();
    }

    outfile->Close();
}
