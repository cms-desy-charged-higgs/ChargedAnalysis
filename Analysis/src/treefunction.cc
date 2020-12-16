#include <ChargedAnalysis/Analysis/include/treefunction.h>

TreeFunction::TreeFunction(std::shared_ptr<TFile>& inputFile, const std::string& treeName, const int& era, const bool& useSyst) :
    inputFile(inputFile),
    inputTree(inputFile->Get<TTree>(treeName.c_str())),
    era(era),
    useSyst(useSyst) {
    funcInfo = {
        {"Pt", {&TreeFunction::Pt, "p_{T}(@) [GeV]"}},
        {"Mt", {&TreeFunction::Mt, "m_{T}(@) [GeV]"}},
        {"Phi", {&TreeFunction::Phi, "#phi(@) [rad]"}},
        {"Eta", {&TreeFunction::Eta, "#eta(@) [rad]"}},
        {"Mass", {&TreeFunction::Mass, "m(@) [GeV]"}},
        {"dR", {&TreeFunction::DeltaR, "#Delta R(@, @) [rad]"}},
        {"dPhi", {&TreeFunction::DeltaPhi, "#Delta #phi(@, @) [rad]"}},
        {"Tau", {&TreeFunction::JetSubtiness, "#tau_{@}(@)"}},
        {"HT", {&TreeFunction::HT, "H_{T} [GeV]"}},
        {"N", {&TreeFunction::NParticle, "N(@)"}},
        {"EvNr", {&TreeFunction::EventNumber, "Event number"}},
        {"HTag", {&TreeFunction::HTag, "Higgs score(@)"}},
        {"DAK8", {&TreeFunction::DeepAK, "DeepAK8 score(@)"}},
        {"DAK8C", {&TreeFunction::DeepAKClass, "DeepAK8 class"}},
        {"DNN", {&TreeFunction::DNN, "DNN score @ (m_{H^{#pm}} = @ GeV)"}},
        {"DNNC", {&TreeFunction::DNNClass, "DNN class (m_{H^{#pm}} = @ GeV)"}},
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

    systInfo = {
        {"BTag", BTAG},
        {"ElectronID", ELEID},
        {"ElectronReco", ELERECO},
        {"MuonIso", MUISO},
        {"MuonID", MUID},
        {"MuonTriggerEff", MUTRIGG},
    };
}

TreeFunction::~TreeFunction(){}

const bool TreeFunction::hasYAxis(){return yFunction != nullptr;}

void TreeFunction::SetYAxis(){
    if(this == nullptr) yFunction = std::make_shared<TreeFunction>(inputFile, inputTree->GetName());
}

template<Axis A>
void TreeFunction::SetP1(const std::string& part, const int& idx, const std::string& wp, const int& genMother){
    if(A == Axis::Y){
        yFunction->SetP1<Axis::X>(part, idx, wp, genMother);
        return;
    }

    part1 = std::get<0>(VUtil::At(partInfo, part));
    wp1 = std::get<0>(VUtil::At(wpInfo, wp));
    idx1 = idx-1;
    genMother1 = genMother;

    partLabel1 = std::get<3>(VUtil::At(partInfo, part));
    if(!StrUtil::Find(partLabel1, "@").empty())
        partLabel1 = StrUtil::Replace(partLabel1, "@", idx1 == -1. ? "" : std::to_string(idx1+1));
    partName1 = part;
    wpName1 = wp;
}

template void TreeFunction::SetP1<Axis::X>(const std::string&, const int&, const std::string&, const int&);
template void TreeFunction::SetP1<Axis::Y>(const std::string&, const int&, const std::string&, const int&);

template<Axis A>
void TreeFunction::SetP2(const std::string& part, const int& idx, const std::string& wp, const int& genMother){
    if(A == Axis::Y){
        yFunction->SetP2<Axis::X>(part, idx, wp);
        return;
    }

    part2 = std::get<0>(VUtil::At(partInfo, part));
    wp2 = std::get<0>(VUtil::At(wpInfo, wp));
    idx2 = idx-1;
    genMother2 = genMother;

    partLabel2 = std::get<3>(VUtil::At(partInfo, part));
    if(!StrUtil::Find(partLabel2, "@").empty())
        partLabel2 = StrUtil::Replace(partLabel2, "@", idx1 == -1. ? "" : std::to_string(idx2+1));
    partName2 = part;
    wpName2 = wp;
}

template void TreeFunction::SetP2<Axis::X>(const std::string&, const int&, const std::string&, const int&);
template void TreeFunction::SetP2<Axis::Y>(const std::string&, const int&, const std::string&, const int&);

template<Axis A>
void TreeFunction::SetCleanJet(const std::string& part, const std::string& wp){
    if(A == Axis::Y){
        yFunction->SetCleanJet<Axis::X>(part, wp);
        return;
    }

    if(part1 == JET or part1 == BJET){
        cleanPhi = RUtil::Get<TLeaf>(inputTree, StrUtil::Replace("@_Phi", "@", std::get<1>(VUtil::At(partInfo, part))));
        cleanEta =  RUtil::Get<TLeaf>(inputTree, StrUtil::Replace("@_Eta", "@", std::get<1>(VUtil::At(partInfo, part))));
        ID = RUtil::Get<TLeaf>(inputTree, StrUtil::Replace("@_ID", "@", std::get<1>(VUtil::At(partInfo, part))));
        Isolation =  RUtil::Get<TLeaf>(inputTree, StrUtil::Replace(part == "e" ? "@_Isolation" : "@_isoID", "@", std::get<1>(VUtil::At(partInfo, part))));

        jetPhi = RUtil::Get<TLeaf>(inputTree, "Jet_Phi");
        jetEta = RUtil::Get<TLeaf>(inputTree, "Jet_Eta");
        cleanPart = std::get<0>(VUtil::At(partInfo, part));
        cleanedWP = std::get<0>(VUtil::At(wpInfo, wp));
    }
}

template void TreeFunction::SetCleanJet<Axis::X>(const std::string&, const std::string&);
template void TreeFunction::SetCleanJet<Axis::Y>(const std::string&, const std::string&);

void TreeFunction::SetCut(const std::string& comp, const float& compValue){
    this->comp = std::get<0>(VUtil::At(comparisons, comp));
    this->compValue = compValue;

    std::string compV = std::to_string(compValue);
    compV.erase(compV.find_last_not_of('0') + 1, std::string::npos); 
    compV.erase(compV.find_last_not_of('.') + 1, std::string::npos);

    cutLabel = axisLabel + " " + std::get<1>(VUtil::At(comparisons, comp)) + " " + compV;
}

void TreeFunction::SetSystematics(const std::vector<std::string>& systNames){
    if(systNames.size() == 1) return;

    for(const std::string syst : VUtil::Slice(systNames, 1, systNames.size())){
        systematics.push_back(VUtil::At(systInfo, syst));
    }

    systWeights = std::vector<float>(systematics.size()*2, 1.);
}

template<Axis A>
void TreeFunction::SetFunction(const std::string& funcName, const std::string& inputValue){
    if(A == Axis::Y){
        yFunction->SetFunction<Axis::X>(funcName, inputValue);
        return;
    }

    if(genMother1 != -1.){
        particleID = RUtil::Get<TLeaf>(inputTree, "GenParticle_ParticleID");
        motherID = RUtil::Get<TLeaf>(inputTree, "GenParticle_MotherID");
    }

    this->funcPtr = std::get<0>(VUtil::At(funcInfo, funcName));
    this->inputValue = inputValue;

    std::string part1Prefix = genMother1 == -1. ? std::get<1>(partInfo[partName1]) : "GenParticle";
    std::string part2Prefix = genMother1 == -1. ? std::get<1>(partInfo[partName2]) : "GenParticle";

    if(funcName == "Pt"){
        const std::string branchName = StrUtil::Replace("@_Pt", "@", part1Prefix);
        TLeaf* leaf = RUtil::Get<TLeaf>(inputTree, branchName);
        quantities.push_back(std::move(leaf));
    }

    else if(funcName == "Mt"){
        const std::string branchName = StrUtil::Replace("@_Mt", "@", part1Prefix);
        TLeaf* leaf = RUtil::Get<TLeaf>(inputTree, branchName);
        quantities.push_back(std::move(leaf));
    }

    else if(funcName == "Eta"){
        const std::string branchName = StrUtil::Replace("@_Eta", "@", part1Prefix);
        TLeaf* leaf = RUtil::Get<TLeaf>(inputTree, branchName);
        quantities.push_back(std::move(leaf));
    }

    else if(funcName == "Phi"){
        const std::string branchName = StrUtil::Replace("@_Phi", "@", part1Prefix);
        TLeaf* leaf = RUtil::Get<TLeaf>(inputTree, branchName);
        quantities.push_back(std::move(leaf));
    }

    else if(funcName == "Mass"){
        const std::string branchName = StrUtil::Replace("@_Mass", "@", part1Prefix);
        TLeaf* leaf = RUtil::Get<TLeaf>(inputTree, branchName);
        quantities.push_back(std::move(leaf));
    }

    else if(funcName == "dPhi"){
        for(const std::string part : {part1Prefix, part2Prefix}){
            TLeaf* phi = RUtil::Get<TLeaf>(inputTree, StrUtil::Replace("@_Phi", "@", part));
            quantities.push_back(std::move(phi));
        }
    }

    else if(funcName == "dR"){
        for(const std::string part : {part1Prefix, part2Prefix}){
            TLeaf* phi = RUtil::Get<TLeaf>(inputTree, StrUtil::Replace("@_Phi", "@", part));
            TLeaf* eta = RUtil::Get<TLeaf>(inputTree, StrUtil::Replace("@_Eta", "@", part));
            quantities.push_back(std::move(phi));
            quantities.push_back(std::move(eta));
        }
    }

    else if(funcName == "HT"){
        for(const std::string& jetName : {"Jet", "FatJet"}){
            const std::string branchName = StrUtil::Replace("@_Pt", "@", jetName);
            TLeaf* leaf = RUtil::Get<TLeaf>(inputTree, branchName);
            quantities.push_back(std::move(leaf));
        }
    }

    else if(funcName == "Tau"){
        const std::string branchName = StrUtil::Replace("@_Njettiness" + inputValue, "@", part1Prefix);
        TLeaf* leaf = RUtil::Get<TLeaf>(inputTree, branchName);
        quantities.push_back(std::move(leaf));
    }

    else if(funcName == "HTag"){
        const std::string branchName = "ML_HTagFJ" + std::to_string(idx1+1);
        TLeaf* leaf = RUtil::Get<TLeaf>(inputTree, branchName);
        quantities.push_back(std::move(leaf));
    }

    else if(funcName == "DNN"){
        const std::string branchName = StrUtil::Replace("DNN_@", "@", inputValue);
        TLeaf* leaf = RUtil::Get<TLeaf>(inputTree, branchName);
        quantities.push_back(std::move(leaf));
    }


    else if(funcName == "DNNC"){
        for(const std::string p : {"HPlus", "TT1L", "TT2L", "Misc"}){
            const std::string branchName = StrUtil::Replace("DNN_@@", "@", p, inputValue);
            TLeaf* leaf = RUtil::Get<TLeaf>(inputTree, branchName);
            quantities.push_back(std::move(leaf));
        }
    }

    else if(funcName == "DAK8"){
        std::string branchName = StrUtil::Replace("@_DeepAK8@", "@", part1Prefix, inputValue);
        TLeaf* leaf = RUtil::Get<TLeaf>(inputTree, branchName);
        quantities.push_back(std::move(leaf));
    }

    else if(funcName == "DAK8C"){
        for(const std::string p : {"Higgs", "Top", "W", "QCD"}){
            std::string branchName = StrUtil::Replace("@_DeepAK8@", "@", part1Prefix, p);
            TLeaf* leaf = RUtil::Get<TLeaf>(inputTree, branchName);
            quantities.push_back(std::move(leaf));
        }
    }

    else if(funcName == "EvNr"){
        TLeaf* leaf = RUtil::Get<TLeaf>(inputTree, "Misc_eventNumber");
        quantities.push_back(std::move(leaf));
    }

    else if(funcName == "N"){
        if(part1 == BJET or part1 == BSUBJET){
            for(const std::string& param : {"Pt", "Eta"}){
                const std::string branchName = std::string(part1 == BJET ? "Jet" : "SubJet") + "_" + param;
                TLeaf* leaf = RUtil::Get<TLeaf>(inputTree, branchName);
                quantities.push_back(std::move(leaf));
            }
        }
    }

    const std::string branchName1 = StrUtil::Replace("@_Size", "@", part1Prefix);
    nPart1 = inputTree->GetLeaf(branchName1.c_str());

    const std::string branchName2 = StrUtil::Replace("@_Size", "@", part2Prefix);
    nPart2 = inputTree->GetLeaf(branchName2.c_str());

    if(wp1 != NONE){
        std::string wpname = std::get<1>(wpInfo[wpName1]);

        switch(part1){
            case ELECTRON:
                ID = RUtil::Get<TLeaf>(inputTree, "Electron_ID");
                Isolation = RUtil::Get<TLeaf>(inputTree, "Electron_Isolation");
                scaleFactors[ELERECO] = RUtil::Get<TLeaf>(inputTree, "Electron_recoSF");
                if(useSyst) scaleFactorsUp[ELERECO] = RUtil::Get<TLeaf>(inputTree, "Electron_recoSFUp");
                if(useSyst) scaleFactorsDown[ELERECO] = RUtil::Get<TLeaf>(inputTree, "Electron_recoSFDown");

                scaleFactors[ELEID] = RUtil::Get<TLeaf>(inputTree, StrUtil::Replace("Electron_@SF", "@", wpname));
                if(useSyst) scaleFactorsUp[ELEID] = RUtil::Get<TLeaf>(inputTree, StrUtil::Replace("Electron_@SFUp", "@", wpname));
                if(useSyst) scaleFactorsDown[ELEID] = RUtil::Get<TLeaf>(inputTree, StrUtil::Replace("Electron_@SFDown", "@", wpname));

                break;

            case MUON:
                ID = RUtil::Get<TLeaf>(inputTree, "Muon_ID");
                Isolation = RUtil::Get<TLeaf>(inputTree, "Muon_isoID");

                scaleFactors[MUTRIGG] = RUtil::Get<TLeaf>(inputTree, "Muon_triggerSF");
                if(useSyst) scaleFactorsUp[MUTRIGG] = RUtil::Get<TLeaf>(inputTree, "Muon_triggerSFUp");
                if(useSyst) scaleFactorsDown[MUTRIGG] = RUtil::Get<TLeaf>(inputTree, "Muon_triggerSFDown");

                scaleFactors[MUID] =  RUtil::Get<TLeaf>(inputTree, StrUtil::Replace("Muon_@SF", "@", wpname));
                if(useSyst) scaleFactorsUp[MUID] = RUtil::Get<TLeaf>(inputTree, StrUtil::Replace("Muon_@SFUp", "@", wpname));
                if(useSyst) scaleFactorsDown[MUID] = RUtil::Get<TLeaf>(inputTree, StrUtil::Replace("Muon_@SFDown", "@", wpname));

                wpname[0] = std::toupper(wpname[0]); 
                scaleFactors[MUISO] =  RUtil::Get<TLeaf>(inputTree, StrUtil::Replace("Muon_tightIso@SF", "@", wpname));
                if(useSyst) scaleFactorsUp[MUISO] = RUtil::Get<TLeaf>(inputTree, StrUtil::Replace("Muon_tightIso@SFUp", "@", wpname));
                if(useSyst) scaleFactorsDown[MUISO] = RUtil::Get<TLeaf>(inputTree, StrUtil::Replace("Muon_tightIso@SFDown", "@", wpname));
                break;

            case BJET:
                TrueFlavour = RUtil::Get<TLeaf>(inputTree, "Jet_TrueFlavour");
                BScore = RUtil::Get<TLeaf>(inputTree, "Jet_CSVScore");
                scaleFactors[BTAG] = RUtil::Get<TLeaf>(inputTree, StrUtil::Replace("Jet_@CSVbTagSF", "@", wpname));
                if(useSyst) scaleFactorsUp[BTAG] = RUtil::Get<TLeaf>(inputTree, StrUtil::Replace("Jet_@CSVbTagSFUp", "@", wpname));
                if(useSyst) scaleFactorsDown[BTAG] = RUtil::Get<TLeaf>(inputTree, StrUtil::Replace("Jet_@CSVbTagSFDown", "@", wpname));
                break;

            case BSUBJET:
                TrueFlavour = RUtil::Get<TLeaf>(inputTree, "SubJet_TrueFlavour");
                BScore = RUtil::Get<TLeaf>(inputTree, "SubJet_CSVScore");
                scaleFactors[SUBBTAG] = RUtil::Get<TLeaf>(inputTree, StrUtil::Replace("SubJet_@CSVbTagSF", "@", wpname));
                if(useSyst) scaleFactorsUp[SUBBTAG] = RUtil::Get<TLeaf>(inputTree, StrUtil::Replace("SubJet_@CSVbTagSFUp", "@", wpname));
                if(useSyst) scaleFactorsDown[SUBBTAG] = RUtil::Get<TLeaf>(inputTree, StrUtil::Replace("SubJet_@CSVbTagSFDown", "@", wpname));
                break;

            default: break;
        }

    }

    if(part1 == BJET or part1 == BSUBJET){
        std::string wpname = std::get<1>(wpInfo[wpName1]);
        wpname[0] = std::toupper(wpname[0]);

        if(inputFile->Get("nTrueB") != nullptr){
            effB = (TH2F*)RUtil::Get<TH2F>(inputFile.get(), StrUtil::Replace("n@BCSVbTag", "@", wpname))->Clone();
            effB->Divide(RUtil::Get<TH2F>(inputFile.get(), "nTrueB"));
            effC = (TH2F*)RUtil::Get<TH2F>(inputFile.get(), StrUtil::Replace("n@CCSVbTag", "@", wpname))->Clone();
            effC->Divide(RUtil::Get<TH2F>(inputFile.get(), "nTrueC"));
            effLight = (TH2F*)RUtil::Get<TH2F>(inputFile.get(), StrUtil::Replace("n@LightCSVbTag", "@", wpname))->Clone();
            effLight->Divide(RUtil::Get<TH2F>(inputFile.get(), "nTrueLight"));
        }
    }

    //Set Name of functions/axis label
    name = funcName + (inputValue != "" ? StrUtil::Replace("_@", "@", inputValue) : "") + (partName1 != "" ? "_" + std::get<2>(VUtil::At(partInfo, partName1)) : "") + (idx1 != -1 ? "_" + std::to_string(idx1+1) : "") + (wp1 != NONE ? "_" + std::get<1>(VUtil::At(wpInfo, wpName1)) : "") + (partName2 != "" ? "_" + std::get<2>(VUtil::At(partInfo, partName2)) : "") + (idx2 != -1 ? "_" + std::to_string(idx2+1) : "") + (wp2 != NONE ? "_" + std::get<1>(VUtil::At(wpInfo, wpName2)) : "");
    axisLabel = (genMother1 != -1. ? "Gen " : "") + std::get<1>(VUtil::At(funcInfo, funcName));

    if(inputValue != ""){
        if(!StrUtil::Find(axisLabel, "@").empty()){
            if(funcName == "DNN"){
                axisLabel = StrUtil::Replace(axisLabel, "@", inputValue.substr(0, inputValue.size() - 3), inputValue.substr(inputValue.size() - 3, inputValue.size()));  
            }

            else axisLabel = StrUtil::Replace(axisLabel, "@", inputValue);   
        }
    }

    for(const std::string partLabel : {partLabel1, partLabel2}){
        if(!StrUtil::Find(axisLabel, "@").empty()){
            axisLabel = StrUtil::Replace(axisLabel, "@", partLabel);
        }
    }
}

template void TreeFunction::SetFunction<Axis::X>(const std::string&, const std::string&);
template void TreeFunction::SetFunction<Axis::Y>(const std::string&, const std::string&);

void TreeFunction::SetEntry(const int& entry){
    TreeFunction::entry = entry;
}

template<Axis A>
const float TreeFunction::Get(){
    if(A == Axis::Y) return yFunction->Get<Axis::X>();

    realIdx1 = whichIndex(nPart1, part1, idx1, wp1, genMother1);
    if(part2 != VACUUM) realIdx2 = whichIndex(nPart2, part2, idx2, wp2, genMother2);

    try{
        (this->*funcPtr)();
        return value;
    } 

    catch(const std::exception& e){
        return -999.;    
    }
}

template const float TreeFunction::Get<Axis::X>();
template const float TreeFunction::Get<Axis::Y>();

const float TreeFunction::GetWeight(const Systematic syst, const Shift& shift){
    weight = 1.;

    if(funcPtr == &TreeFunction::NParticle){
        float wData = 1., wMC = 1.;

        if((part1 == BJET or part1 == BSUBJET) and effB != nullptr){
            TLeaf* scaleFactor;

            if(shift != NON and (syst == BTAG or syst == SUBBTAG)){
                if(shift == UP) scaleFactor = scaleFactorsUp[part1 == BJET ? BTAG : SUBBTAG];   
                else scaleFactor = scaleFactorsDown[part1 == BJET ? BTAG : SUBBTAG];  
            }

            else scaleFactor = scaleFactors[part1 == BJET ? BTAG : SUBBTAG];

            const std::vector<float>& sf = RUtil::GetVecEntry<float>(scaleFactor, entry);
            const std::vector<char>& trueFlav = RUtil::GetVecEntry<char>(TrueFlavour, entry);

            const std::vector<float>& jetPt = RUtil::GetVecEntry<float>(quantities[0], entry);
            const std::vector<float>& jetEta = RUtil::GetVecEntry<float>(quantities[1], entry);

            for(unsigned int i = 0; i < sf.size(); i++){
                float eff;
    
                if(std::abs(trueFlav.at(i)) == 5) eff = effB->GetBinContent(effB->FindBin(VUtil::At(jetPt, i), VUtil::At(jetEta, i)));
                else if(std::abs(trueFlav.at(i)) == 4) eff = effC->GetBinContent(effC->FindBin(VUtil::At(jetPt, i), VUtil::At(jetEta, i)));
                else eff = effLight->GetBinContent(effLight->FindBin(VUtil::At(jetPt, i), VUtil::At(jetEta, i)));

                if(eff == 0) continue;

                if(whichWP(part1, i) >= wp1){
                    wData *= VUtil::At(sf, i) * eff;
                    wMC *= eff;
                }
    
                else{
                    wData *= 1 - VUtil::At(sf, i)*eff;
                    wMC *= (1 - eff);
                }
            }
            weight = wData/wMC;
        }

        else{
            for(const Systematic& sys: VUtil::MapKeys(scaleFactors)){
                TLeaf* scaleFactor;

                if(shift != NON and sys == syst){
                    if(shift == UP) scaleFactor = scaleFactorsUp[sys];    
                    else scaleFactor = scaleFactorsDown[sys];  
                }

                else scaleFactor = scaleFactors[sys];
       
                const std::vector<float>& sf = RUtil::GetVecEntry<float>(scaleFactor, entry);

                for(int i = 0; i < sf.size(); i++){
                    if(whichWP(part1, i) >= wp1){
                        weight *= Utils::CheckZero(VUtil::At(sf, i));  
                    }
                }
            }
        }
    }

    return weight;
}

const std::vector<float> TreeFunction::GetSystWeight(){
    for(int i = 0; i < systematics.size(); i++){
        VUtil::At(systWeights, MUtil::RowMajIdx({systematics.size(), 2}, {i, 0})) = this->GetWeight(systematics[i], UP);
        VUtil::At(systWeights, MUtil::RowMajIdx({systematics.size(), 2}, {i, 1})) = this->GetWeight(systematics[i], DOWN);
    }

    return systWeights;
}

const bool TreeFunction::GetPassed(){
    switch(comp){
        case BIGGER:
            return this->Get<Axis::X>() > compValue;

        case SMALLER:
            return this->Get<Axis::X>() < compValue;

        case EQUAL:
            return this->Get<Axis::X>() == compValue;

        case DIVISIBLE:
            return int(this->Get<Axis::X>()) % int(compValue) == 0;

        case NOTDIVISIBLE:
            return int(this->Get<Axis::X>()) % int(compValue) != 0;
    }
}

template<Axis A>
const std::string TreeFunction::GetAxisLabel(){
    if(A == Axis::Y) return yFunction->GetAxisLabel<Axis::X>();

    return axisLabel;
}

template const std::string TreeFunction::GetAxisLabel<Axis::X>();
template const std::string TreeFunction::GetAxisLabel<Axis::Y>();

const std::string TreeFunction::GetCutLabel(){
    return cutLabel;
}

template<Axis A>
const std::string TreeFunction::GetName(){
    if(A == Axis::Y) return yFunction->GetName<Axis::X>();
    return name;
}

int TreeFunction::whichIndex(TLeaf* nPart, const Particle& part, const int& idx, const WP& wp, const int& genMother){
    if((nPart == nullptr or idx == -1) and genMother == -1.) return -1.;

    int realIdx = 0, counter = idx;

    if(genMother != -1.){
        const std::vector<char>& partID = RUtil::GetVecEntry<char>(particleID, entry);
        const std::vector<char>& mothID = RUtil::GetVecEntry<char>(motherID, entry);
   
        for(int i = 0; i < partID.size(); i++){
            if(VUtil::At(partID, i) == part and VUtil::At(mothID, i) == genMother){
                counter--;
                realIdx = i;
                if(counter <= 0) return realIdx;
            }
        }
    }

    else{
        const char& n = RUtil::GetEntry<char>(nPart1, entry);
        if(idx >= n) return -1.;
        
        while(counter != 0){
            if(whichWP(part, realIdx) >= wp) counter--;
   
            realIdx++; 
            if(realIdx >= n) return -1.;
        }

        return realIdx;
    }
}

TreeFunction::WP TreeFunction::whichWP(const Particle& part, const int& idx){
    std::vector<char> id, isoID;
    std::vector<float> iso, score;

    switch(part){
        case ELECTRON:
            id = RUtil::GetVecEntry<char>(ID, entry);
            iso = RUtil::GetVecEntry<float>(Isolation, entry);

            if(VUtil::At(id, idx) > MEDIUM && VUtil::At(iso, idx) < 0.15) return TIGHT;
            if(VUtil::At(id, idx) > LOOSE && VUtil::At(iso, idx) < 0.20) return MEDIUM;
            if(VUtil::At(id, idx) > NONE && VUtil::At(iso, idx) < 0.25) return LOOSE;
            return NONE;

        case MUON:
            id = RUtil::GetVecEntry<char>(ID, entry);
            isoID = RUtil::GetVecEntry<char>(Isolation, entry);

            if(VUtil::At(id, idx) > MEDIUM && VUtil::At(isoID, idx) > MEDIUM) return TIGHT;
            if(VUtil::At(id, idx) > LOOSE && VUtil::At(isoID, idx) > MEDIUM) return MEDIUM;
            if(VUtil::At(id, idx) > NONE && VUtil::At(isoID, idx) > MEDIUM) return LOOSE;
            return NONE;


        case JET:
            if(cleanPart != VACUUM){
                if(isCleanJet(idx)) return NONE;
                else return NOTCLEAN;
            }

            return NONE;

        case BJET:
            if(cleanPart != VACUUM){
                if(!isCleanJet(idx)) return NOTCLEAN;
            }

        case BSUBJET:
            score = RUtil::GetVecEntry<float>(BScore, entry);

            if(VUtil::At(score, idx) > tightBCut[era]) return TIGHT;
            if(VUtil::At(score, idx) > mediumBCut[era]) return MEDIUM;
            if(VUtil::At(score, idx) > looseBCut[era]) return LOOSE;

        default: return NONE;
    }
}

template const std::string TreeFunction::GetName<Axis::X>();
template const std::string TreeFunction::GetName<Axis::Y>();

bool TreeFunction::isCleanJet(const int& idx){
    const std::vector<float>& phi = RUtil::GetVecEntry<float>(cleanPhi, entry);
    const std::vector<float>& eta = RUtil::GetVecEntry<float>(cleanEta, entry);
    const std::vector<float>& jPhi = RUtil::GetVecEntry<float>(jetPhi, entry);
    const std::vector<float>& jEta = RUtil::GetVecEntry<float>(jetEta, entry);

    for(unsigned int i = 0; i < phi.size(); i++){
        if(whichWP(cleanPart, i) < cleanedWP) continue;
        if(std::sqrt(std::pow(VUtil::At(phi, i) - VUtil::At(jPhi, idx), 2) + std::pow(VUtil::At(eta, i) - VUtil::At(jEta, idx), 2)) < 0.4) return false;        
    }

    return true;
}

void TreeFunction::Pt(){
    if(realIdx1 != -1) value = VUtil::At(RUtil::GetVecEntry<float>(quantities[0], entry), realIdx1);
    else value = RUtil::GetEntry<float>(quantities[0], entry);
}

void TreeFunction::Phi(){
    if(realIdx1 != -1) value = VUtil::At(RUtil::GetVecEntry<float>(quantities[0], entry), realIdx1);
    else value = RUtil::GetEntry<float>(quantities[0], entry);
}

void TreeFunction::Mass(){
    if(realIdx1 != -1) value = VUtil::At(RUtil::GetVecEntry<float>(quantities[0], entry), realIdx1);
    else value = RUtil::GetEntry<float>(quantities[0], entry);
}

void TreeFunction::Mt(){
    value = RUtil::GetEntry<float>(quantities[0], entry);
}

void TreeFunction::Eta(){
    if(realIdx1 != -1) value = VUtil::At(RUtil::GetVecEntry<float>(quantities[0], entry), realIdx1);
    else value = RUtil::GetEntry<float>(quantities[0], entry);
}

void TreeFunction::DeltaPhi(){
    float phi1 = realIdx1 != -1 ? VUtil::At(RUtil::GetVecEntry<float>(quantities[0], entry), realIdx1) : RUtil::GetEntry<float>(quantities[0], entry);
    float phi2 = realIdx2 != -1 ? VUtil::At(RUtil::GetVecEntry<float>(quantities[1], entry), realIdx2) : RUtil::GetEntry<float>(quantities[1], entry);

    value = std::acos(std::cos(phi1)*std::cos(phi2) + std::sin(phi1)*std::sin(phi2));
}

void TreeFunction::DeltaR(){
    float phi1 = realIdx1 != -1 ? VUtil::At(RUtil::GetVecEntry<float>(quantities[0], entry), realIdx1) : RUtil::GetEntry<float>(quantities[0], entry);
    float phi2 = realIdx2 != -1 ? VUtil::At(RUtil::GetVecEntry<float>(quantities[1], entry), realIdx2) : RUtil::GetEntry<float>(quantities[1], entry);
    float eta1 = realIdx1 != -1 ? VUtil::At(RUtil::GetVecEntry<float>(quantities[2], entry), realIdx1) : RUtil::GetEntry<float>(quantities[2], entry);
    float eta2 = realIdx2 != -1 ? VUtil::At(RUtil::GetVecEntry<float>(quantities[3], entry), realIdx2) : RUtil::GetEntry<float>(quantities[3], entry);

    value = std::sqrt(std::pow(phi1-phi2, 2) + std::pow(eta1-eta2, 2));
}

void TreeFunction::JetSubtiness(){
    value = VUtil::At(RUtil::GetVecEntry<float>(quantities[0], entry), realIdx1);
}

void TreeFunction::EventNumber(){
    value = Utils::BitCount(int(RUtil::GetEntry<float>(quantities[0], entry)));
}

void TreeFunction::HTag(){
    value = RUtil::GetEntry<float>(quantities[0], entry);
}

void TreeFunction::DeepAK(){
    value = VUtil::At(RUtil::GetVecEntry<float>(quantities[0], entry), realIdx1);
}

void TreeFunction::DeepAKClass(){
    float max = 0;
    int maxIdx = 0;

    for(int i = 0; i < quantities.size(); ++i){
        const float& v = VUtil::At(RUtil::GetVecEntry<float>(quantities[i], entry), realIdx1);
        if(v > max){
            maxIdx = i;
            max = v;
        }
    }

    value = maxIdx;
}

void TreeFunction::DNNClass(){
    float max = 0;
    int maxIdx = 0;

    for(int i = 0; i < quantities.size(); ++i){
        const float& v = RUtil::GetEntry<float>(quantities[i], entry);
        if(v > max){
            maxIdx = i;
            max = v;
        }
    }
  
    value = maxIdx;
}

void TreeFunction::DNN(){
    value = RUtil::GetEntry<float>(quantities[0], entry);
}

void TreeFunction::HT(){
    value = 0;

    for(TLeaf* jet : quantities){
        const std::vector<float>& jetPt = RUtil::GetVecEntry<float>(jet, entry);

        for(const float& pt : jetPt){
            value += pt;
        }
    }
}

void TreeFunction::NParticle(){
    value = 0;

    if(part1 == ELECTRON or part1 == MUON){
        const std::vector<char>& id = RUtil::GetVecEntry<char>(ID, entry);

        for(int i = 0; i < id.size(); i++){
            if(whichWP(part1, i) >= wp1) value++;       
        }
    }

    else if(part1 == JET){
        const char& n = RUtil::GetEntry<char>(nPart1, entry);

        for(int i = 0; i < n; ++i){
            if(whichWP(part1, i) >= wp1) value++;       
        }
    }

    else if(part1 == BJET or part1 == BSUBJET){
        const std::vector<float>& score = RUtil::GetVecEntry<float>(BScore, entry);

        for(int i = 0; i < score.size(); i++){
            if(whichWP(part1, i) >= wp1) value++;       
        }
    }

    else{
        value = RUtil::GetEntry<char>(nPart1, entry);
    }
}
