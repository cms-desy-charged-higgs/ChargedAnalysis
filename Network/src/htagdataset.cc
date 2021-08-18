/**
* @file htagdataset.cc
* @brief Source file for HTagDataset class, see htagdataset.h
*/

#include <ChargedAnalysis/Network/include/htagdataset.h>

HTagDataset::HTagDataset(std::shared_ptr<TFile>& inFile, std::shared_ptr<TTree>& inTree, const std::vector<std::string>& cutNames, const std::string& cleanJet, const int& fatIndex, torch::Device& device, const bool& isSignal, const int& matchedPart) :
    inTree(inTree),
    fatIndex(fatIndex),
    device(device),
    isSignal(isSignal),
    matchedPart(matchedPart){

    /*
    TreeParser parser;
    std::vector<TreeFunction> cuts;
    std::vector<std::string> cleanInfo = StrUtil::Split(cleanJet, "/");

    for(const std::string& cutName: cutNames){
        if(StrUtil::Find(cutName, inTree->GetName()).empty()) continue;

        //Functor structure and arguments
        TreeFunction cut(inFile, inTree->GetName());
        
        //Read in everything, orders matter
        parser.GetParticle(cutName, cut);
        parser.GetFunction(cutName, cut);
        parser.GetCut(cutName, cut);

        if(cleanInfo.size() != 1){
            cut.SetCleanJet<Axis::X>(cleanInfo[0], cleanInfo[1]);    
        }
        
        cuts.push_back(cut);
    }

    TLeaf* jetTrue = inTree->GetLeaf("FatJet_ParticleID");

    for(int i = 0; i < inTree->GetEntries(); i++){
        TreeFunction::SetEntry(i);

        bool passed=true;

        if(matchedPart != -1){
            jetTrue->GetBranch()->GetEntry(i);
            std::vector<char>* trueValue = static_cast<std::vector<char>*>(jetTrue->GetValuePointer());

            if(trueValue->at(fatIndex) == matchedPart) passed = false;
        }

        for(TreeFunction& cut : cuts){
            if(!passed) break;

            passed = passed && cut.GetPassed();
        }

        if(passed){
            trueIndex.push_back(i);
            nEntries++; 
        }
    }

    std::vector<std::string> partVar = {"Pt", "Eta", "Phi", "Mass", "Vx", "Vy", "Vz"};
    
    for(std::string& var: partVar){
        jetPart.push_back(inTree->GetLeaf(("JetParticle_" + var).c_str()));
    }

    for(std::string& var: partVar){
        vtx.push_back(inTree->GetLeaf(("SecondaryVertex_" + var).c_str()));
    }

    fatJetPt = inTree->GetLeaf("FatJet_Pt");
    jetCharge = inTree->GetLeaf("JetParticle_Charge");
    jetIdx = inTree->GetLeaf("JetParticle_FatJetIdx");
    vtxIdx = inTree->GetLeaf("SecondaryVertex_FatJetIdx");
    */
}

torch::optional<size_t> HTagDataset::size() const {
    return nEntries;
}
        
HTensor HTagDataset::get(size_t index){
    int entry = trueIndex[index];
    fatJetPt->GetBranch()->GetEntry(entry);  
    float fatPt = ((std::vector<float>*)fatJetPt->GetValuePointer())->at(fatIndex);

    jetCharge->GetBranch()->GetEntry(entry);
    jetIdx->GetBranch()->GetEntry(entry);
    vtxIdx->GetBranch()->GetEntry(entry);

    std::vector<char>* charge = (std::vector<char>*)jetCharge->GetValuePointer();
    std::vector<char>* idx = (std::vector<char>*)jetIdx->GetValuePointer();
    std::vector<char>* vIdx = (std::vector<char>*)vtxIdx->GetValuePointer();

    int nParts = charge->size();

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

    std::vector<int> sortByPt;
    std::function<bool(float, float)> sortPt = [&](float v1, float v2){return v1 > v2;};

    //Fill vec of jet parts with (E1, PX1, .., VZ1, E2, PX2.. VZN)
    for(int i = 0; i < jetPart.size(); i++){
        jetPart[i]->GetBranch()->GetEntry(entry);
        std::vector<float>* partVar = (std::vector<float>*)jetPart[i]->GetValuePointer();

        if(i == 0) sortByPt = VUtil::SortedIndices(*partVar, sortPt);
        std::vector<float> vec = VUtil::SortByIndex(*partVar, sortByPt);
        if(i == 0) vec = VUtil::Transform<float>(vec, [&](float i){return i/fatPt;});  

        int nC = 0; int nN = 0;

        for(int j = 0; j < nParts; j++){
            if(idx->at(j) == fatIndex){
                if(charge->at(j) != 0){
                    chargedParticles.at(i + jetPart.size()*nC) = vec.at(j);
                    nC++;
                }

                else{
                    neutralParticles.at(i + jetPart.size()*nN) = vec.at(j);
                    nN++;
                }
            }
        } 
    }

    for(int i = 0; i < vtx.size(); i++){
        vtx[i]->GetBranch()->GetEntry(entry);
        std::vector<float>* partVar = (std::vector<float>*)vtx[i]->GetValuePointer();
    
        if(partVar->size() == 0) break;

        if(i == 0) sortByPt = VUtil::SortedIndices(*partVar, sortPt);
        std::vector<float> vec = VUtil::SortByIndex(*partVar, sortByPt);
        if(i == 0) vec = VUtil::Transform<float>(vec, [&](float i){return i/fatPt;});

        int nV = 0;

        for(int j = 0; j < partVar->size(); j++){
            if(vIdx->at(j) == fatIndex){
                SV.at(i + vtx.size()*nV) = vec.at(j);
                nV++;
            }
        }
    }

    //Do padding if no SV is there
    if(SV.empty()) SV = std::vector<float>(7, 0);

    torch::Tensor chargedTensor = torch::from_blob(chargedParticles.data(), {1, nCharged, 7}).clone().to(device);
    torch::Tensor neutralTensor = torch::from_blob(neutralParticles.data(), {1, nNeutral, 7}).clone().to(device);
    torch::Tensor SVTensor = torch::from_blob(SV.data(), {1, nVtx != 0 ? nVtx : 1, 7}).clone().to(device);

    return {chargedTensor, neutralTensor, SVTensor, torch::tensor({float(isSignal)}).to(device)};
}

HTensor HTagDataset::PadAndMerge(std::vector<HTensor>& tensors){
    int charMax = 0; int neutralMax = 0; int SVMax = 0;
    std::vector<torch::Tensor> charged, neutral, SV, label; 

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
    }


    return {torch::cat(charged, 0), torch::cat(neutral, 0), torch::cat(SV, 0), torch::cat(label, 0)};
}
