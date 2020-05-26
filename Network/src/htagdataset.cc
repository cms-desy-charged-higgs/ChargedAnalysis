#include <ChargedAnalysis/Network/include/htagdataset.h>

HTagDataset::HTagDataset(const std::vector<std::string>& files, const int& fatIndex, torch::Device& device, const bool& isSignal) :
    fatIndex(fatIndex),
    device(device),
    isSignal(isSignal){

    chain = new TChain();

    //Load  files to chain
    for(const std::string& file: files){
        chain->Add(file.c_str());
    }
}

HTagDataset::~HTagDataset(){}

torch::optional<size_t> HTagDataset::size() const {
    return chain->GetEntries();
}
        
HTensor HTagDataset::get(size_t index){
    int entry = chain->LoadTree(index);
    TLeaf* evNr = chain->GetLeaf("Misc_eventNumber");
    jetPart.clear();
    vtx.clear();

    std::vector<std::string> partVar = {"Pt", "Eta", "Phi", "Mass", "Vx", "Vy", "Vz"};
    
    for(std::string& var: partVar){
        jetPart.push_back(chain->GetLeaf(("JetParticle_" + var).c_str()));
    }

    jetCharge = chain->GetLeaf("JetParticle_Charge");
    jetIdx = chain->GetLeaf("JetParticle_FatJetIdx");
    vtxIdx = chain->GetLeaf("SecondaryVertex_FatJetIdx");

    for(std::string& var: partVar){
        vtx.push_back(chain->GetLeaf(("SecondaryVertex_" + var).c_str()));
    }

    evNr->GetBranch()->GetEntry(entry);
    jetCharge->GetBranch()->GetEntry(entry);
    jetIdx->GetBranch()->GetEntry(entry);
    vtxIdx->GetBranch()->GetEntry(entry);

    std::vector<char>* charge = (std::vector<char>*)jetCharge->GetValuePointer();
    std::vector<char>* idx = (std::vector<char>*)jetIdx->GetValuePointer();
    std::vector<char>* vIdx = (std::vector<char>*)vtxIdx->GetValuePointer();

    int nParts = charge->size() > 100 ? 100 : charge->size();

    int nCharged = 0, nNeutral = 0, nVtx = 0;

    for(int i = 0; i < nParts; i++){
        if(idx->at(i) == fatIndex){
            if(charge->at(i) != 0) nCharged++;
            else nNeutral++;
        }
    }

    for(int i = 0; i < vIdx->size(); i++){
        if(vIdx->at(i) == fatIndex) nVtx++;
    }

    std::vector<float> chargedParticles(nCharged * jetPart.size(), 0), neutralParticles(nNeutral * jetPart.size(), 0), SV(nVtx * jetPart.size(), 0);

    //Fill vec of jet parts with (E1, PX1, .., VZ1, E2, PX2.. VZN)
    for(int i = 0; i < jetPart.size(); i++){
        jetPart[i]->GetBranch()->GetEntry(entry);
        std::vector<float>* partVar = (std::vector<float>*)jetPart[i]->GetValuePointer(); 

        int nC = 0; int nN = 0;

        for(int j = 0; j < nParts; j++){
            if(idx->at(j) == fatIndex){
                if(charge->at(j) != 0){
                    chargedParticles.at(i + jetPart.size()*nC) = partVar->at(j);
                    nC++;
                }

                else{
                    neutralParticles.at(i + jetPart.size()*nN) = partVar->at(j);
                    nN++;
                }
            }
        } 
    }

    for(int i = 0; i < vtx.size(); i++){
        vtx[i]->GetBranch()->GetEntry(entry);
        std::vector<float>* partVar = (std::vector<float>*)vtx[i]->GetValuePointer();

        int nV = 0;

        for(int j = 0; j < partVar->size(); j++){
            if(vIdx->at(j) == fatIndex){
                SV.at(i + 7*nV) = partVar->at(j);
                nV++;
            }
        }
    }

    //Do padding if no SV is there
    if(SV.empty()) SV = std::vector<float>(7, 0);

    int isEven = Utils::BitCount(int(*(float*)evNr->GetValuePointer())) % 2 == 0;

    torch::Tensor chargedTensor = torch::from_blob(chargedParticles.data(), {1, nCharged, 7}).clone().to(device);
    torch::Tensor neutralTensor = torch::from_blob(neutralParticles.data(), {1, nNeutral, 7}).clone().to(device);
    torch::Tensor SVTensor = torch::from_blob(SV.data(), {1, nVtx != 0 ? nVtx : 1, 7}).clone().to(device);

    return {chargedTensor, neutralTensor, SVTensor, torch::tensor({float(isSignal)}).to(device), torch::tensor({isEven}).to(device)};
}

HTensor HTagDataset::PadAndMerge(std::vector<HTensor>& tensors){
    int charMax = 0; int neutralMax = 0; int SVMax = 0;
    std::vector<torch::Tensor> charged, neutral, SV, label, isEven; 

    for(HTensor& tensor: tensors){
        if(tensor.charged.size(1) > charMax) charMax = tensor.charged.size(1);
        if(tensor.neutral.size(1) > neutralMax) neutralMax = tensor.neutral.size(1);
        if(tensor.SV.size(1) > SVMax) SVMax = tensor.SV.size(1);
    }

    for(HTensor& tensor: tensors){
        if(tensor.charged.size(1) < charMax){
            tensor.charged = torch::constant_pad_nd(tensor.charged, {0,0, 0, charMax - tensor.charged.size(1)}, 0);
        }
        charged.push_back(tensor.charged);

        if(tensor.neutral.size(1) < neutralMax){
            tensor.neutral = torch::constant_pad_nd(tensor.neutral, {0,0, 0, neutralMax - tensor.neutral.size(1)}, 0);
        }
        neutral.push_back(tensor.neutral);

        if(tensor.SV.size(1) < SVMax){
            tensor.SV = torch::constant_pad_nd(tensor.SV, {0,0, 0, SVMax - tensor.SV.size(1)}, 0);
        }
        SV.push_back(tensor.SV);
        label.push_back(tensor.label);
        isEven.push_back(tensor.isEven);
    }


    return {torch::cat(charged, 0), torch::cat(neutral, 0), torch::cat(SV, 0), torch::cat(label, 0), torch::cat(isEven, 0)};
}

void HTagDataset::clear(){delete chain;}

