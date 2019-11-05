#include <ChargedAnalysis/Network/include/htagger.h>

HTagger::HTagger(){}

HTagger::HTagger(const int& nFeat, const int& nHidden, const int& nLSTM, const int& nConvFilter, const int& kernelSize, const float& dropOut):
    nLSTM(nLSTM),
    nHidden(nHidden),
    dropOut(dropOut)
    {

    //String for model summary
    modelSummary << "\n";
    modelSummary << "Summary of HTagger Model\n";
    modelSummary << "----------------------------------------\n";

    //Register LSTM Layers
    lstmCharged = register_module("LSTM charged input", torch::nn::LSTM(torch::nn::LSTMOptions(nFeat, nHidden).layers(nLSTM).batch_first(true).dropout(dropOut)));
    lstmCharged->pretty_print(modelSummary); modelSummary << "\n";

    lstmNeutral = register_module("LSTM neutral input", torch::nn::LSTM(torch::nn::LSTMOptions(nFeat, nHidden).layers(nLSTM).batch_first(true).dropout(dropOut)));
    lstmNeutral->pretty_print(modelSummary); modelSummary << "\n";

    lstmSV = register_module("LSTM SV input", torch::nn::LSTM(torch::nn::LSTMOptions(nFeat, nHidden).layers(nLSTM).batch_first(true).dropout(dropOut)));
    lstmSV->pretty_print(modelSummary); modelSummary << "\n";

    //Convolution layer
    convLayer = register_module("Conv layer 1", torch::nn::Conv1d(3, nConvFilter, kernelSize));
    convLayer->pretty_print(modelSummary); modelSummary << "\n";

    //Output layer
    int outConv = nHidden + 2 - kernelSize -1;
    outLayer = register_module("Output layer", torch::nn::Linear(nConvFilter*outConv, 1));
    outLayer->pretty_print(modelSummary); modelSummary << "\n";

    //Number of trainable parameters
    int nWeights = 0;

    for(at::Tensor t: this->parameters()){
        if(t.requires_grad()) nWeights += t.numel();
    }

    modelSummary << "Number of trainable parameters: " << nWeights << "\n";  
}

void HTagger::Print(){
    //Print model summary
    modelSummary << "----------------------------------------\n";
    std::cout << modelSummary.str() << std::endl;
}

torch::Tensor HTagger::forward(torch::Tensor inputCharged, torch::Tensor inputNeutral, torch::Tensor inputSV, const bool& isTraining){
    //Check device
    torch::Device device(torch::cuda::is_available() ? torch::kCUDA : torch::kCPU);

    //Pack into pytorch class so LSTM skipped padded values
    torch::Tensor chargedSeq = ((inputCharged.narrow(2, 0, 1) != 0).sum(1) - 1).squeeze(1);
    torch::Tensor neutralSeq = ((inputNeutral.narrow(2, 0, 1) != 0).sum(1) - 1).squeeze(1);
    torch::Tensor SVSeq = ((inputSV.narrow(2, 0, 1) != 0).sum(1) - 1).squeeze(1);

    torch::Tensor zero = torch::tensor({0}, torch::kLong).to(device);

    chargedSeq = torch::where(chargedSeq == -1, zero, chargedSeq);
    neutralSeq = torch::where(neutralSeq == -1, zero, neutralSeq);
    SVSeq = torch::where(SVSeq == -1, zero, SVSeq);

    torch::Tensor batchIndex = torch::arange(0, inputCharged.size(0), torch::kLong).to(device);

    //LSTM layer
    torch::nn::RNNOutput charged = lstmCharged->forward(inputCharged);
    torch::nn::RNNOutput neutral = lstmNeutral->forward(inputNeutral);
    torch::nn::RNNOutput SV = lstmSV->forward(inputSV);
    
    //Get last hidden state after layer N
    torch::Tensor chargedHidden = charged.output.index({batchIndex, chargedSeq}).unsqueeze(1);
    torch::Tensor neutralHidden = neutral.output.index({batchIndex,  neutralSeq}).unsqueeze(1);
    torch::Tensor SVHidden = SV.output.index({batchIndex, SVSeq}).unsqueeze(1);

    //Merge them together
    torch::Tensor z = torch::cat({chargedHidden, neutralHidden, SVHidden}, 1);

    //Convolution layer
    z = convLayer->forward(z);
    z = torch::dropout(z, dropOut, isTraining);
    z = torch::relu(z).view({-1, z.size(1)*z.size(2)});

    //Output layer
    z = outLayer->forward(z);
    z = torch::dropout(z, dropOut, isTraining);
    z = torch::sigmoid(z);

    return z;
}

std::vector<torch::Tensor> HTagger::GatherInput(const std::string& fileName, const std::string& channel, const int& entryStart, const int& entryEnd, const int& FJindex){
    if(entryEnd == 0){
        return {torch::empty({0}), torch::empty({0}), torch::empty({0}), torch::empty({0}), torch::empty({0})};
    }

    //Input ROOT TTree
    TFile* inputFile = TFile::Open(fileName.c_str(), "READ");
    TTree* eventTree = (TTree*)inputFile->Get(channel.c_str());

    //Variables names and vectors to set branch address to
    std::vector<std::string> particleVariables = {"E", "Px", "Py", "Pz", "Vx", "Vy", "Vz", "Charge", "FatJetIdx"};

    std::vector<std::vector<float>*> particleVec(particleVariables.size(), NULL);
    std::vector<std::vector<float>*> secVtxVec(particleVariables.size(), NULL);

    //Set all addresses
    for(unsigned int idx=0; idx < particleVariables.size(); idx++){
        eventTree->SetBranchAddress(("JetParticle_" + particleVariables[idx]).c_str(), &particleVec[idx]);
        eventTree->SetBranchAddress(("SecondaryVertex_" + particleVariables[idx]).c_str(), &secVtxVec[idx]);
    }

    std::vector<float>* jetPx = NULL;
    std::vector<float>* jetPy = NULL;

    eventTree->SetBranchAddress("FatJet_Px", &jetPx);
    eventTree->SetBranchAddress("FatJet_Py", &jetPy);

    //Save FJ1 and FJ2 jet particles in pseudo matrix (IsPhiUp/FatJetIndex/Vector of Particles)
    std::vector<torch::Tensor> chargedTensors; 
    std::vector<torch::Tensor> neutralTensors; 
    std::vector<torch::Tensor> SVTensors; 

    //Save fo FJ1 and FJ2 NMax particles
    int nCharged = 0;
    int nNeutral = 0;
    int nSV = 0;

    //Vector to check is event number is even or odd
    std::vector<long> phiUp; 
    std::vector<long> phiDown; 
    int index = 0;

    for (int i = entryStart; i < entryEnd; i++){
        eventTree->GetEntry(i); 
        
        float phi = ROOT::Math::PxPyPzEVector(jetPx->at(0), jetPy->at(0), 0, 0).Phi();
         
        if(phi > 0) phiUp.push_back(index);
        else phiDown.push_back(index);      

        //Vector to save for FJ1 and FJ2 to PF Cand (FatJetIndex/Vector of Particles)
        std::vector<float> chargedParticles;
        std::vector<float> neutralParticles;
        std::vector<float> SV;

        //Fill vec of jet parts with (E1, PX1, .., VZ1, E2, PX2.. VZN) for each FatJet
        for(unsigned int idx=0; idx < particleVec[0]->size(); idx++){
            if(particleVec[7]->at(idx) != 0 and particleVec[8]->at(idx) == FJindex){
                for(int j=0; j < 7; j++){
                    chargedParticles.push_back(particleVec[j]->at(idx));
                }
            }

            else if(particleVec[8]->at(idx) == FJindex){
                for(int j=0; j < 7; j++){
                    neutralParticles.push_back(particleVec[j]->at(idx));
                }
            }
        }

        for(unsigned int idx=0; idx < secVtxVec[0]->size(); idx++){
            if(secVtxVec[8]->at(idx) == FJindex){
                for(int j=0; j < 7; j++){
                    SV.push_back(secVtxVec[j]->at(idx));
                }
            }
        }

        //Do padding if no SV is there
        if(SV.empty()) SV = std::vector<float>(7, 0);

        int tmpCharged = Utils::Ratio(chargedParticles.size(), 7);
        int tmpNeutral = Utils::Ratio(neutralParticles.size(), 7);
        int tmpSV = Utils::Ratio(SV.size(), 7);

        nCharged = tmpCharged > nCharged ? tmpCharged : nCharged;
        nNeutral = tmpNeutral > nNeutral ? tmpNeutral : nNeutral;
        nSV = tmpSV > nSV ? tmpSV : nSV;

        //Fill fat jets particles into event vector
        chargedTensors.push_back(torch::from_blob(chargedParticles.data(), {1, tmpCharged, 7}).clone());
        neutralTensors.push_back(torch::from_blob(neutralParticles.data(), {1, tmpNeutral, 7}).clone());
        SVTensors.push_back(torch::from_blob(SV.data(), {1, tmpSV, 7}).clone());

        index++;
    }

    delete eventTree;
    delete inputFile;

    //Transform to tensors and do padding
    for(int i = 0; i < chargedTensors.size(); i++){
        if(chargedTensors[i].size(1) < nCharged) chargedTensors[i] = torch::constant_pad_nd(chargedTensors[i], {0,0, 0, nCharged - chargedTensors[i].size(1)}, 0);

        if(neutralTensors[i].size(1) < nNeutral) neutralTensors[i] = torch::constant_pad_nd(neutralTensors[i], {0,0, 0, nNeutral - neutralTensors[i].size(1)}, 0);

        if(SVTensors[i].size(1) < nSV) SVTensors[i] = torch::constant_pad_nd(SVTensors[i], {0,0, 0, nSV - SVTensors[i].size(1)}, 0);
    }

    return {torch::cat(chargedTensors, 0), 
            torch::cat(neutralTensors, 0), 
            torch::cat(SVTensors, 0), 
            torch::from_blob(phiUp.data(), {(int)phiUp.size()}, torch::kLong).clone(),
            torch::from_blob(phiDown.data(), {(int)phiDown.size()}, torch::kLong).clone()};
}
