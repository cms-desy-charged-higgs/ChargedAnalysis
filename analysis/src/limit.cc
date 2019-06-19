#include <ChargedHiggs/analysis/interface/limit.h>

Limit::Limit(){}

Limit::Limit(std::vector<std::string> &masses, std::vector<std::string> &channels, std::vector<std::string> &bkgProc):
    masses(masses),
    channels(channels),
    bkgProc(bkgProc){

    sigProc = {"HPlus"};
    bins = {{1, "HighB"}};
}

void Limit::SetSyst(){
      cb.cp().backgrounds().AddSyst(cb, "conversativeSys", "lnN", ch::syst::SystMap<ch::syst::era>::init({"2017"}, 1.3));
      cb.cp().signals().AddSyst(cb, "conversativeSys", "lnN", ch::syst::SystMap<ch::syst::era>::init({"2017"}, 1.3));
}

void Limit::WriteDatacard(){
    cb.AddObservations({"*"}, {"HPlusWh"}, {"2017"}, channels,  bins);
    cb.AddProcesses({"*"}, {"HPlusWh"}, {"2017"}, channels, bkgProc,  bins, false);  
    cb.AddProcesses(masses, {"HPlusWh"}, {"2017"}, channels, sigProc,  bins, true); 

    SetSyst();

    for(std::string proc: bkgProc){
        cb.cp().backgrounds().process({proc}).ExtractShapes("LimitTrees/" + proc + ".root", "m_Hc", "");
    }

    for(std::string mass: masses){
        TFile output(std::string("datacard_input_" + mass + ".root").c_str(), "RECREATE");

        cb.cp().signals().process({"HPlus"}).mass({mass}).ExtractShapes("LimitTrees/L4B_" + mass + "_100.root", "m_Hc", "");
        cb.cp().mass({mass, "*"}).WriteDatacard("datacard_" + mass + ".txt", output);

        output.Close();
    }
    
    cb.PrintAll();
}


