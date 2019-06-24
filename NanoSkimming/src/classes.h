#include <ChargedHiggs/NanoSkimming/interface/nanoskimmer.h>
#include <ChargedHiggs/NanoSkimming/interface/jetanalyzer.h>
#include <ChargedHiggs/NanoSkimming/interface/electronanalyzer.h>
#include <ChargedHiggs/NanoSkimming/interface/muonanalyzer.h>
#include <ChargedHiggs/NanoSkimming/interface/genpartanalyzer.h>

namespace{
    namespace{
        Jet jet;
        FatJet fatjet;
        Electron electron;
        Muon muon;
        GenPart genPart;

        std::vector<Jet> jets;
        std::vector<FatJet> fatjets;
        std::vector<Electron> electrons;
        std::vector<Muon> muons;

        NanoSkimmer skimmer;
    }
}
