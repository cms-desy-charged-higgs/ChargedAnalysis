#include <ChargedAnalysis/Analysis/include/treefunction.h>

TreeFunction::TreeFunction(TFile* inputFile, const std::string& treeName) :
    inputFile(inputFile),
    inputTree(inputFile->Get<TTree>(treeName.c_str())){
    funcInfo = {
        {"Pt", {&TreeFunction::Pt, "p_{T}(@) [GeV]"}},
        {"Phi", {&TreeFunction::Phi, "#phi(@) [rad]"}},
        {"Eta", {&TreeFunction::Eta, "#eta(@) [rad]"}},
        {"Mass", {&TreeFunction::Mass, "m(@) [GeV]"}},
        {"Tau", {&TreeFunction::JetSubtiness, "#tau_{@}(@)"}},
        {"HT", {&TreeFunction::HT, "H_{T} [GeV]"}},
        {"N", {&TreeFunction::NParticle, "N(@)"}},
        {"EvNr", {&TreeFunction::EventNumber, "Event number"}},
        {"HTag", {&TreeFunction::HTag, "Higgs score(@)"}},
        {"DAK8", {&TreeFunction::DeepAK, "DeepAK8 Higgs vs Top(@)"}},
        {"DNN", {&TreeFunction::DNN, "DNN score(m_{H^{#pm}} = @ GeV)"}},
    };

    partInfo = {
        {"", {VACUUM, "", "", ""}},
        {"e", {ELECTRON, "Electron", "Electron", "e_{@}"}},
        {"mu", {MUON, "Muon", "Muon", "#mu_{@}"}},
        {"j", {JET, "Jet", "Jet", "j_{@}"}},
        {"met", {MET, "MET", "MET", "#vec{p}_{T}^{miss}"}},
        {"sj", {SUBJET, "SubJet", "SubJet", "j^{sub}_{@}"}},
        {"bj", {BJET, "Jet", "BJet", "b_{@}"}},
        {"bsj", {BSUBJET, "SubJet", "BSubJet", "b^{sub}_{@}"}},
        {"fj", {FATJET, "FatJet", "FatJet", "j_{@}^{AK8}"}},
        {"h1", {HIGGS, "H1", "H1", "h_{1}"}},
        {"h2", {HIGGS, "H2", "H2", "h_{2}"}},
        {"hc", {CHAREDHIGGS, "HPlus", "HPlus", "H^{#pm}"}},
        {"W", {W, "W", "W", "W^{#pm}"}},
    };

    wpInfo = {
        {"", {NONE, ""}},
        {"l", {LOOSE, "loose"}},
        {"m", {MEDIUM, "medium"}},
        {"t", {TIGHT, "tight"}},
    };

    comparisons = {
        {"bigger", {BIGGER, ">"}},
        {"smaller", {SMALLER, "<"}},
        {"equal", {EQUAL, "=="}},
        {"divisible", {DIVISIBLE, "%"}},
        {"notdivisible", {NOTDIVISIBLE, "%!"}},
    };
}

TreeFunction::~TreeFunction(){}

void TreeFunction::SetP1(const std::string& part, const int& idx, const std::string& wp){
    part1 = std::get<0>(partInfo.at(part));
    wp1 = std::get<0>(wpInfo.at(wp));
    idx1 = idx-1;

    partLabel1 = Utils::Format<std::string>("@", std::get<3>(partInfo.at(part)), idx1 == -1. ? "" : std::to_string(idx1+1), true);
    partName1 = part;
    wpName1 = wp;
}

void TreeFunction::SetP2(const std::string& part, const int& idx, const std::string& wp){
    part2 = std::get<0>(partInfo.at(part));
    wp2 = std::get<0>(wpInfo.at(wp));
    idx2 = idx-1;

    partLabel2 = Utils::Format<std::string>("@", std::get<3>(partInfo.at(part)), idx2 == -1. ? "" : std::to_string(idx2+1), true);
    partName2 = part;
    wpName2 = wp;
}

void TreeFunction::SetCleanJet(const std::string& part, const std::string& wp){
    if(part1 == JET or part1 == BJET){
        cleanPhi = inputTree->GetLeaf(Utils::Format<std::string>("@", "@_Phi", std::get<1>(partInfo.at(part))).c_str());
        cleanEta = inputTree->GetLeaf(Utils::Format<std::string>("@", "@_Eta", std::get<1>(partInfo.at(part))).c_str());
        ID = inputTree->GetLeaf(Utils::Format<std::string>("@", "@_ID", std::get<1>(partInfo.at(part))).c_str());
        Isolation = inputTree->GetLeaf(Utils::Format<std::string>("@", part == "e" ? "@_Isolation" : "@_isoID", std::get<1>(partInfo.at(part))).c_str());

        jetPhi = inputTree->GetLeaf("Jet_Phi");
        jetEta = inputTree->GetLeaf("Jet_Eta");
        cleanPart = std::get<0>(partInfo.at(part));
        cleanedWP = std::get<0>(wpInfo.at(wp));
    }
}

void TreeFunction::SetCut(const std::string& comp, const float& compValue){
    this->comp = std::get<0>(comparisons.at(comp));
    this->compValue = compValue;

    std::string compV = std::to_string(compValue);
    compV.erase(compV.find_last_not_of('0') + 1, std::string::npos); 
    compV.erase(compV.find_last_not_of('.') + 1, std::string::npos);

    cutLabel = axisLabel + " " + std::get<1>(comparisons.at(comp)) + " " + compV;
}

void TreeFunction::SetFunction(const std::string& funcName, const float& inputValue){
    this->funcPtr = std::get<0>(funcInfo.at(funcName));
    this->inputValue = inputValue;

    const std::string& partName = std::get<1>(partInfo[partName1]);

    if(funcName == "Pt"){
        const std::string branchName = Utils::Format<std::string>("@", "@_Pt", partName);
        TLeaf* leaf = Utils::CheckNull<TLeaf>(inputTree->GetLeaf(branchName.c_str()));
        quantities.push_back(std::move(leaf));
    }

    else if(funcName == "Eta"){
        const std::string branchName = Utils::Format<std::string>("@", "@_Eta", partName);
        TLeaf* leaf = Utils::CheckNull<TLeaf>(inputTree->GetLeaf(branchName.c_str()));
        quantities.push_back(std::move(leaf));
    }

    else if(funcName == "Phi"){
        const std::string branchName = Utils::Format<std::string>("@", "@_Phi", partName);
        TLeaf* leaf = Utils::CheckNull<TLeaf>(inputTree->GetLeaf(branchName.c_str()));
        quantities.push_back(std::move(leaf));
    }

    else if(funcName == "Mass"){
        const std::string branchName = Utils::Format<std::string>("@", "@_Mass", partName);
        TLeaf* leaf = Utils::CheckNull<TLeaf>(inputTree->GetLeaf(branchName.c_str()));
        quantities.push_back(std::move(leaf));
    }

    else if(funcName == "HT"){
        for(const std::string& jetName : {"Jet", "FatJet"}){
            const std::string branchName = Utils::Format<std::string>("@", "@_Pt", jetName);
            TLeaf* leaf = Utils::CheckNull<TLeaf>(inputTree->GetLeaf(branchName.c_str()));
            quantities.push_back(std::move(leaf));
        }
    }

    else if(funcName == "Tau"){
        const std::string branchName = Utils::Format<std::string>("@", "@_Njettiness" + std::to_string(int(inputValue)), partName);
        TLeaf* leaf = Utils::CheckNull<TLeaf>(inputTree->GetLeaf(branchName.c_str()));
        quantities.push_back(std::move(leaf));
    }

    else if(funcName == "HTag"){
        const std::string branchName = "ML_HTagFJ" + std::to_string(idx1+1);
        TLeaf* leaf = Utils::CheckNull<TLeaf>(inputTree->GetLeaf(branchName.c_str()));
        quantities.push_back(std::move(leaf));
    }

    else if(funcName == "DNN"){
        const std::string branchName = Utils::Format<int>("@", "ML_DNN@", int(inputValue));
        TLeaf* leaf = Utils::CheckNull<TLeaf>(inputTree->GetLeaf(branchName.c_str()));
        quantities.push_back(std::move(leaf));
    }

    else if(funcName == "DAK8"){
        const std::string branchName = Utils::Format<std::string>("@", "@_topVsHiggs", partName);
        TLeaf* leaf = Utils::CheckNull<TLeaf>(inputTree->GetLeaf(branchName.c_str()));
        quantities.push_back(std::move(leaf));
    }

    else if(funcName == "EvNr"){
        TLeaf* leaf = Utils::CheckNull<TLeaf>(inputTree->GetLeaf("Misc_eventNumber"));
        quantities.push_back(std::move(leaf));
    }

    else if(funcName == "N"){
        if(part1 == BJET or part1 == BSUBJET){
            for(const std::string& param : {"Pt", "Eta"}){
                const std::string branchName = std::string(part1 == BJET ? "Jet" : "SubJet") + "_" + param;
                TLeaf* leaf = Utils::CheckNull<TLeaf>(inputTree->GetLeaf(branchName.c_str()));
                quantities.push_back(std::move(leaf));
            }
        }
    }

    const std::string branchName = Utils::Format<std::string>("@", "@_Size", partName);
    nPart = inputTree->GetLeaf(branchName.c_str());

    if(part1 < JET and wp1 != NONE){
        std::string wpname = std::get<1>(wpInfo[wpName1]);

        switch(part1){
            case ELECTRON:
                ID = Utils::CheckNull<TLeaf>(inputTree->GetLeaf("Electron_ID"));
                Isolation = Utils::CheckNull<TLeaf>(inputTree->GetLeaf("Electron_Isolation"));
                scaleFactors.push_back(Utils::CheckNull<TLeaf>(inputTree->GetLeaf("Electron_recoSF")));
                scaleFactors.push_back(Utils::CheckNull<TLeaf>(inputTree->GetLeaf(Utils::Format<std::string>("@", "Electron_@SF", wpname).c_str())));

                break;

            case MUON:
                ID = Utils::CheckNull<TLeaf>(inputTree->GetLeaf("Muon_ID"));
                Isolation = Utils::CheckNull<TLeaf>(inputTree->GetLeaf("Muon_isoID"));
                scaleFactors.push_back(Utils::CheckNull<TLeaf>(inputTree->GetLeaf("Muon_triggerSF")));
                scaleFactors.push_back(Utils::CheckNull<TLeaf>(inputTree->GetLeaf(Utils::Format<std::string>("@", "Muon_@SF", wpname).c_str())));

                wpname[0] = std::toupper(wpname[0]); 
                scaleFactors.push_back(Utils::CheckNull<TLeaf>(inputTree->GetLeaf(Utils::Format<std::string>("@", "Muon_tightIso@SF", wpname).c_str())));
                break;

            case BJET:
                nTrueB = Utils::CheckNull<TLeaf>(inputTree->GetLeaf("Jet_TrueFlavour"));
                BScore = Utils::CheckNull<TLeaf>(inputTree->GetLeaf("Jet_CSVScore"));
                scaleFactors.push_back(Utils::CheckNull<TLeaf>(inputTree->GetLeaf(Utils::Format<std::string>("@", "Jet_@CSVbTagSF", wpname).c_str())));
                break;

            case BSUBJET:
                nTrueB = Utils::CheckNull<TLeaf>(inputTree->GetLeaf("SubJet_TrueFlavour"));
                BScore = Utils::CheckNull<TLeaf>(inputTree->GetLeaf("SubJet_CSVScore"));
                scaleFactors.push_back(Utils::CheckNull<TLeaf>(inputTree->GetLeaf(Utils::Format<std::string>("@", "SubJet_@CSVbTagSF", wpname).c_str())));
                break;

            default: break;
        }

    }

    if(part1 == BJET or part1 == BSUBJET){
        std::string wpname = std::get<1>(wpInfo[wpName1]);
        wpname[0] = std::toupper(wpname[0]);

        effBTag = inputFile->Get<TH2F>(Utils::Format<std::string>("@", "n@CSVbTag", wpname).c_str());

        if(effBTag != nullptr) effBTag->Divide(inputFile->Get<TH2F>("nTrueB"));
    }

    //Set Name of functions/axis label
    name = funcName + (inputValue != -999. ? Utils::Format<int>("@", "_@", inputValue) : "") + (partName1 != "" ? "_" + std::get<2>(partInfo.at(partName1)) : "") + (idx1 != -1 ? "_" + std::to_string(idx1+1) : "") + (wp1 != NONE ? "_" + std::get<1>(wpInfo.at(wpName1)) : "") + (partName2 != "" ? "_" + std::get<2>(partInfo.at(partName2)) : "") + (idx2 != -1 ? "_" + std::to_string(idx2+1) : "") + (wp2 != NONE ? "_" + std::get<1>(wpInfo.at(wpName2)) : "");
    axisLabel = std::get<1>(funcInfo.at(funcName));;

    if(inputValue != -999.){
        axisLabel = Utils::Format<int>("@", axisLabel, inputValue, true);
    }

    for(const std::string partLabel : {partLabel1, partLabel2}){
        axisLabel = Utils::Format<std::string>("@", axisLabel, partLabel, true);
    }
}

void TreeFunction::SetEntry(const int& entry){
    TreeFunction::entry = entry;
}

const float TreeFunction::Get(){
    if(idx1 != -1){
        if(nPart->GetBranch()->GetReadEntry() != entry) nPart->GetBranch()->GetEntry(entry);
        const char* n = (char*)nPart->GetValuePointer();
             
        if(idx1 >= *n) return -999.;
        realIdx1 = 0; int counter = idx1;

        while(counter != 0){
            if(whichWP(part1, realIdx1) >= wp1) counter--;

            realIdx1++; 
            if(realIdx1 >= *n) return -999.;
        }

        try{
            (this->*funcPtr)();
            return value;
        } 

        catch(const std::exception& e){
            return -999.;    
        }
    }

    else{
        try{
            realIdx1 = -1.;
            (this->*funcPtr)();
            return value;
        } 

        catch(const std::exception& e){
            return value;    
        }
    }
}

const float TreeFunction::GetWeight(){
    weight = 1.;

    if(funcPtr == &TreeFunction::NParticle){
        if((part1 == BJET or part1 == BSUBJET) and effBTag != nullptr){
            if(scaleFactors[0]->GetBranch()->GetReadEntry() != entry) scaleFactors[0]->GetBranch()->GetEntry(entry);
            if(nTrueB->GetBranch()->GetReadEntry() != entry) nTrueB->GetBranch()->GetEntry(entry);
            if(quantities[0]->GetBranch()->GetReadEntry() != entry) quantities[0]->GetBranch()->GetEntry(entry);
            if(quantities[1]->GetBranch()->GetReadEntry() != entry) quantities[1]->GetBranch()->GetEntry(entry);

            std::vector<float>* sf = (std::vector<float>*)scaleFactors[0]->GetValuePointer();
            std::vector<char>* nB = (std::vector<char>*)nTrueB->GetValuePointer();

            std::vector<float>* jetPt = (std::vector<float>*)quantities[0]->GetValuePointer();
            std::vector<float>* jetEta = (std::vector<float>*)quantities[1]->GetValuePointer();

            for(unsigned int i = 0; i < sf->size(); i++){
                const float& eff = effBTag->GetBinContent(effBTag->FindBin(jetPt->at(i), jetEta->at(i)));

                if(eff == 0 or eff == 1) continue;

                if(abs(nB->at(i)) == 5){
                    weight *= sf->at(i)*eff/eff;
                }
    
                else{
                    weight *= (1 - sf->at(i)*eff)/(1 - eff);
                }
            }
        }

        else{
            for(TLeaf* scaleFactor: scaleFactors){
                if(scaleFactor->GetBranch()->GetReadEntry() != entry) scaleFactor->GetBranch()->GetEntry(entry);

                std::vector<float>* sf = (std::vector<float>*)scaleFactor->GetValuePointer();

                for(int i = 0; i < sf->size(); i++){
                    weight *= Utils::CheckZero(sf->at(i));
                }
            }
        }
    }

    return weight;
}

const bool TreeFunction::GetPassed(){
    switch(comp){
        case BIGGER:
            return this->Get() > compValue;

        case SMALLER:
            return this->Get() < compValue;

        case EQUAL:
            return this->Get() == compValue;

        case DIVISIBLE:
            return int(this->Get()) % int(compValue) == 0;

        case NOTDIVISIBLE:
            return int(this->Get()) % int(compValue) != 0;
    }
}

const std::string TreeFunction::GetAxisLabel(){
    return axisLabel;
}

const std::string TreeFunction::GetCutLabel(){
    return cutLabel;
}

const std::string TreeFunction::GetName(){
    return name;
}

TreeFunction::WP TreeFunction::whichWP(const Particle& part, const int& idx){
    std::vector<char>* id;
    std::vector<char>* isoID;
    std::vector<float>* iso; 
    std::vector<float>* score;

    switch(part){
        case ELECTRON:
            if(ID->GetBranch()->GetReadEntry() != entry) ID->GetBranch()->GetEntry(entry);
            if(Isolation->GetBranch()->GetReadEntry() != entry) Isolation->GetBranch()->GetEntry(entry);

            id = (std::vector<char>*)ID->GetValuePointer();
            iso = (std::vector<float>*)Isolation->GetValuePointer();

            if(id->at(idx) > MEDIUM && iso->at(idx) < 0.15) return TIGHT;
            if(id->at(idx) > LOOSE && iso->at(idx) < 0.20) return MEDIUM;
            if(id->at(idx) > NONE && iso->at(idx) < 0.25) return LOOSE;
            return NONE;

        case MUON:
            if(ID->GetBranch()->GetReadEntry() != entry) ID->GetBranch()->GetEntry(entry);
            if(Isolation->GetBranch()->GetReadEntry() != entry) Isolation->GetBranch()->GetEntry(entry);

            id = (std::vector<char>*)ID->GetValuePointer();
            isoID = (std::vector<char>*)Isolation->GetValuePointer();

            if(id->at(idx) > MEDIUM && isoID->at(idx) > MEDIUM) return TIGHT;
            if(id->at(idx) > LOOSE && isoID->at(idx) > MEDIUM) return MEDIUM;
            if(id->at(idx) > NONE && isoID->at(idx) > MEDIUM) return LOOSE;
            return NONE;


        case JET:
            if(cleanPhi != nullptr){
                if(isCleanJet(idx)) return NONE;
                else return NOTCLEAN;
            }

            return NONE;

        case BJET:
            if(cleanPhi != nullptr){
                if(!isCleanJet(idx)) return NOTCLEAN;
            }

        case BSUBJET:
            if(BScore->GetBranch()->GetReadEntry() != entry) BScore->GetBranch()->GetEntry(entry);
            score = (std::vector<float>*)BScore->GetValuePointer();

            /*
            if(score->at(idx) > 0.7489) return TIGHT;
            if(score->at(idx) > 0.3033) return MEDIUM;
            if(score->at(idx) > 0.0521) return LOOSE;
            */

            if(score->at(idx) > 0.8001) return TIGHT;
            if(score->at(idx) > 0.4941) return MEDIUM;
            if(score->at(idx) > 0.1522) return LOOSE;

        default: return NONE;
    }
}

bool TreeFunction::isCleanJet(const int& idx){
    if(cleanPhi->GetBranch()->GetReadEntry() != entry) cleanPhi->GetBranch()->GetEntry(entry);
    if(cleanEta->GetBranch()->GetReadEntry() != entry) cleanEta->GetBranch()->GetEntry(entry);
    if(jetPhi->GetBranch()->GetReadEntry() != entry) jetPhi->GetBranch()->GetEntry(entry);
    if(jetEta->GetBranch()->GetReadEntry() != entry) jetEta->GetBranch()->GetEntry(entry);

    const std::vector<float>* phi = (std::vector<float>*)cleanPhi->GetValuePointer();
    const std::vector<float>* eta = (std::vector<float>*)cleanEta->GetValuePointer();
    const std::vector<float>* jPhi = (std::vector<float>*)jetPhi->GetValuePointer();
    const std::vector<float>* jEta = (std::vector<float>*)jetEta->GetValuePointer();

    for(unsigned int i = 0; i < phi->size(); i++){
        if(whichWP(cleanPart, i) < cleanedWP) continue;
        if(std::sqrt(std::pow(phi->at(i) - jPhi->at(idx), 2) + std::pow(eta->at(i) - jEta->at(idx), 2)) < 0.4) return false;        
    }

    return true;
}

void TreeFunction::Pt(){
    if(quantities[0]->GetBranch()->GetReadEntry() != entry) quantities[0]->GetBranch()->GetEntry(entry);

    if(realIdx1 != -1){
        const std::vector<float>* pt = (std::vector<float>*)quantities[0]->GetValuePointer();
        value = pt->at(realIdx1);
    }

    else{
        const float* pt = (float*)quantities[0]->GetValuePointer();
        value = *pt;
    }
}

void TreeFunction::Phi(){
    if(quantities[0]->GetBranch()->GetReadEntry() != entry) quantities[0]->GetBranch()->GetEntry(entry);

    if(realIdx1 != -1){
        const std::vector<float>* phi = (std::vector<float>*)quantities[0]->GetValuePointer();
        value = phi->at(realIdx1);
    }

    else{
        const float* phi = (float*)quantities[0]->GetValuePointer();
        value = *phi;
    }
}

void TreeFunction::Mass(){
    if(quantities[0]->GetBranch()->GetReadEntry() != entry) quantities[0]->GetBranch()->GetEntry(entry);

    if(realIdx1 != -1){
        const std::vector<float>* mass = (std::vector<float>*)quantities[0]->GetValuePointer();
        value = mass->at(realIdx1);
    }

    else{
        const float* mass = (float*)quantities[0]->GetValuePointer();
        value = *mass;
    }
}

void TreeFunction::Eta(){
    if(quantities[0]->GetBranch()->GetReadEntry() != entry) quantities[0]->GetBranch()->GetEntry(entry);

    if(realIdx1 != -1){
        const std::vector<float>* eta = (std::vector<float>*)quantities[0]->GetValuePointer();
        value = eta->at(realIdx1);
    }

    else{
        const float* eta = (float*)quantities[0]->GetValuePointer();
        value = *eta;
    }
}

void TreeFunction::JetSubtiness(){
    if(quantities[0]->GetBranch()->GetReadEntry() != entry) quantities[0]->GetBranch()->GetEntry(entry);
    const std::vector<float>* tau = (std::vector<float>*)quantities[0]->GetValuePointer();

    value = tau->at(realIdx1);
}

void TreeFunction::EventNumber(){
    if(quantities[0]->GetBranch()->GetReadEntry() != entry) quantities[0]->GetBranch()->GetEntry(entry);
    const float* evNr = (float*)quantities[0]->GetValuePointer();

    value = Utils::BitCount(int(*evNr));
}

void TreeFunction::HTag(){
    if(quantities[0]->GetBranch()->GetReadEntry() != entry) quantities[0]->GetBranch()->GetEntry(entry);
    const float* htag = (float*)quantities[0]->GetValuePointer();

    value = *htag;
}

void TreeFunction::DeepAK(){
    if(quantities[0]->GetBranch()->GetReadEntry() != entry) quantities[0]->GetBranch()->GetEntry(entry);
    const std::vector<float>* score = (std::vector<float>*)quantities[0]->GetValuePointer();

    value = score->at(realIdx1);
}

void TreeFunction::DNN(){
    if(quantities[0]->GetBranch()->GetReadEntry() != entry) quantities[0]->GetBranch()->GetEntry(entry);
    const float* score = (float*)quantities[0]->GetValuePointer();

    value = *score;
}

void TreeFunction::HT(){
    value = 0;

    for(const TLeaf* jet : quantities){
        if(jet->GetBranch()->GetReadEntry() != entry) jet->GetBranch()->GetEntry(entry);
        const std::vector<float>* jetPt = (std::vector<float>*)jet->GetValuePointer();

        for(const float& pt : *jetPt){
            value += pt;
        }
    }
}

void TreeFunction::NParticle(){
    value = 0;

    if(part1 == ELECTRON or part1 == MUON){
        if(ID->GetBranch()->GetReadEntry() != entry) ID->GetBranch()->GetEntry(entry);
        std::vector<char>* id = (std::vector<char>*)ID->GetValuePointer();

        for(int i = 0; i < id->size(); i++){
            if(whichWP(part1, i) >= wp1) value++;       
        }
    }

    else if(part1 == JET){
        if(nPart->GetBranch()->GetReadEntry() != entry) nPart->GetBranch()->GetEntry(entry);
        const char* n = (char*)nPart->GetValuePointer();

        for(int i = 0; i < *n; i++){
            if(whichWP(part1, i) >= wp1) value++;       
        }

        value = *n;
    }

    else if(part1 == BJET or part1 == BSUBJET){
        if(BScore->GetBranch()->GetReadEntry() != entry) BScore->GetBranch()->GetEntry(entry);
        std::vector<float>* score = (std::vector<float>*)BScore->GetValuePointer();

        for(int i = 0; i < score->size(); i++){
            if(whichWP(part1, i) >= wp1) value++;       
        }
    }

    else{
        if(nPart->GetBranch()->GetReadEntry() != entry) nPart->GetBranch()->GetEntry(entry);
        const char* n = (char*)nPart->GetValuePointer();

        value = *n;
    }
}
