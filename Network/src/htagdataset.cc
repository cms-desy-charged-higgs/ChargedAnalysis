#include <ChargedAnalysis/Network/include/htagdataset.h>

HTagDataset::HTagDataset(const std::vector<std::string>& files, const std::vector<std::string>& channels, const int& fatIndex, torch::Device& device, const bool& isSignal) :
    fatIndex(fatIndex),
    device(device),
    isSignal(isSignal){

    chain = new TChain();

    //Load  files to chain
    for(const std::string& channel: channels){
        for(const std::string& file: files){
            chain->Add((file + "/" + channel).c_str());
        }
    }

    chain->LoadTree(0);

    //Variables names and vectors to set branch address to
    particleVariables = {"E", "Px", "Py", "Pz", "Vx", "Vy", "Vz", "Charge", "FatJetIdx"};

    particleVec = std::vector<std::vector<float>*>(particleVariables.size(), NULL);
    secVtxVec = std::vector<std::vector<float>*>(particleVariables.size(), NULL);
}

HTagDataset::~HTagDataset(){}

torch::optional<size_t> HTagDataset::size() const {
    return chain->GetEntries();
}
        
HTensor HTagDataset::get(size_t index){
    //Set all addresses
    for(unsigned int idx=0; idx < particleVec.size(); idx++){
        chain->SetBranchAddress(("JetParticle_" + particleVariables[idx]).c_str(), &particleVec[idx]);
        if(particleVariables[idx] != "Charge"){
            chain->SetBranchAddress(("SecondaryVertex_" + particleVariables[idx]).c_str(), &secVtxVec[idx]);
        }
    }

    chain->SetBranchAddress("Misc_eventNumber", &eventNumber);
    chain->GetEntry(index);

    std::vector<float> chargedParticles, neutralParticles, SV;

    //Fill vec of jet parts with (E1, PX1, .., VZ1, E2, PX2.. VZN)
    for(unsigned int idx=0; idx < particleVec[0]->size(); idx++){
        if(particleVec[7]->at(idx) != 0 and particleVec[8]->at(idx) == fatIndex){
            for(int j=0; j < 7; j++){
                chargedParticles.push_back(particleVec[j]->at(idx));
            }
        }

        else if(particleVec[8]->at(idx) == fatIndex){
            for(int j=0; j < 7; j++){
                neutralParticles.push_back(particleVec[j]->at(idx));
            }
        }   
    }

    for(unsigned int idx=0; idx < secVtxVec[0]->size(); idx++){
        if(secVtxVec[8]->at(idx) == fatIndex){
            for(int j=0; j < 7; j++){
                SV.push_back(secVtxVec[j]->at(idx));
            }
        }
    }

    //Do padding if no SV is there
    if(SV.empty()) SV = std::vector<float>(7, 0);

    int isEven = Utils::BitCount(int(eventNumber)) % 2 == 0;

    torch::Tensor chargedTensor = torch::from_blob(chargedParticles.data(), {1, chargedParticles.size()/7., 7}).clone().to(device); 
    torch::Tensor neutralTensor = torch::from_blob(neutralParticles.data(), {1, neutralParticles.size()/7., 7}).clone().to(device); 
    torch::Tensor SVTensor = torch::from_blob(SV.data(), {1, SV.size()/7., 7}).clone().to(device);

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
