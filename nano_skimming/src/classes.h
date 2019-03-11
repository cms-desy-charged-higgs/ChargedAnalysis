#include <ChargedHiggs/nano_skimming/interface/nanoskimmer.h>
#include <ChargedHiggs/nano_skimming/interface/jetanalyzer.h>
#include <ChargedHiggs/nano_skimming/interface/electronanalyzer.h>
#include <ChargedHiggs/nano_skimming/interface/muonanalyzer.h>
#include <ChargedHiggs/nano_skimming/interface/genpartanalyzer.h>

namespace{
    namespace{
        Jet jet;
        Electron electron;
        Muon muon;
        GenPart genPart;

        std::vector<Jet> jets;
        std::vector<Electron> electrons;
        std::vector<Muon> muons;

        NanoSkimmer skimmer;
    }
}
