#include <ChargedHiggs/Analysis/interface/limit.h>

Limit::Limit(){}

Limit::Limit(std::string &mass, std::vector<std::string> &channels, std::vector<std::string> &bkgProc, std::string &outDir):
    mass(mass),
    channels(channels),
    bkgProc(bkgProc),
    outDir(outDir){

    sigProc = {"HPlus"};

    chanToDir = {
            {"mu4j", "Muon4J/LimitHist"},
            {"mu2j1f", "Muon2J1F/LimitHist"}, 
            {"mu2f", "Muon2F/LimitHist"},
            {"e4j", "Ele4J/LimitHist"},
            {"e2j1f", "Ele2J1F/LimitHist"}, 
            {"e2f", "Ele2F/LimitHist"},
    };

    for(unsigned int i=0; i < channels.size(); i++){
        bins.push_back({i+1, channels[i]});    
    }
}

void Limit::SetSyst(){
      cb.cp().backgrounds().AddSyst(cb, "conversativeSys", "lnN", ch::syst::SystMap<ch::syst::era>::init({"2017"}, 1.3));
      cb.cp().signals().AddSyst(cb, "conversativeSys", "lnN", ch::syst::SystMap<ch::syst::era>::init({"2017"}, 1.3));
}

void Limit::WriteDatacard(std::string &histDir, std::string &parameter){
    cb.AddObservations({"*"}, {"HPlusWh"}, {"2017"}, {"*"},  bins);
    cb.AddProcesses({"*"}, {"HPlusWh"}, {"2017"}, {"*"}, bkgProc,  bins, false);  
    cb.AddProcesses({mass}, {"HPlusWh"}, {"2017"}, {"*"}, sigProc,  bins, true); 

    SetSyst();

    TFile output(std::string(outDir + "/datacard_input.root").c_str(), "RECREATE");

    for(std::string channel: channels){
        for(std::string proc: bkgProc){
            cb.cp().backgrounds().process({proc}).bin({channel}).ExtractShapes(histDir + "/" + chanToDir[channel] + "/" + mass + "/" + proc + ".root", parameter, "");
        }

        cb.cp().signals().process({"HPlus"}).bin({channel}).ExtractShapes(histDir + "/" + chanToDir[channel] + "/" + mass + "/L4B_" + mass + "_100.root", parameter, "");
    }

    cb.cp().mass({mass, "*"}).WriteDatacard(outDir + "/datacard.txt", output);
    output.Close();
}

void Limit::CalcLimit(){
    std::system(("combine -d " + outDir + "/datacard.txt -M AsymptoticLimits --mass " + mass).c_str());
    std::system(("mv higgsCombineTest.AsymptoticLimits.mH" + mass + ".root " + outDir + "/limit.root").c_str());

    std::system(("text2workspace.py " + outDir + "/datacard.txt --mass " + mass).c_str());
    std::system(("combine -M FitDiagnostics " + outDir + "/datacard.txt --saveShapes --saveNormalizations --saveWithUncertainties --expectSignal 0 --mass " + mass + " -n " + mass).c_str());
    std::system("command rm -f higgs*.FitDiagnostics.mH* combine_logger.out");
    std::system(("mv fitDiagnostics" + mass + ".root " + outDir + "/fitDiagnostics.root").c_str());
    std::system(("PostFitShapesFromWorkspace -d " + outDir + "/datacard.txt -o "+ outDir + "/fitshapes.root -f " + outDir + "/fitDiagnostics.root:fit_b --sampling --postfit -w " + outDir + "/datacard.root --mass " + mass).c_str());
}


