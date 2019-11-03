#include <ChargedAnalysis/Analysis/include/treeappender.h>

TreeAppender::TreeAppender() {}

TreeAppender::TreeAppender(const std::string& oldFile, const std::string& oldTree, const std::string& newFile, const std::vector<std::string>& branchNames, const int& entryStart, const int& entryEnd) :
        oldFile(oldFile), 
        oldTree(oldTree),
        newFile(newFile),
        branchNames(branchNames),
        entryStart(entryStart), 
        entryEnd(entryEnd) {}

std::vector<float> TreeAppender::HScore(const int& FJindex){
    //Vector with final score (FatJetIndex/Score per event)
    std::vector<float> tagValues(entryEnd-entryStart, -999.);

    //
    std::vector<torch::Tensor> input = HTagger::GatherInput(oldFile, oldTree, entryStart, entryEnd, FJindex);

    //Tagger
    torch::Device device(torch::cuda::is_available() ? torch::kCUDA : torch::kCPU);
    std::vector<std::shared_ptr<HTagger>> tagger(2, std::make_shared<HTagger>(7, 150, 1, 27, 20, 0.22));

    torch::Tensor chargedTensor = input[0];
    torch::Tensor neutralTensor = input[1];
    torch::Tensor SVTensor = input[2];

    //Loop for PhiUp/PhiDown
    for(int n = 0; n < 2; n++){
        torch::Tensor tIndex = n == 0 ? input[3] : input[4];

        //Skip if no events are there
        if(tIndex.size(0) == 0) continue;

        torch::Tensor charged = chargedTensor.index({tIndex});
        torch::Tensor neutral = neutralTensor.index({tIndex});
        torch::Tensor SV = SVTensor.index({tIndex});

        //Load Model
        std::string taggerName = n == 0 ? "ForPhiUp.pt" : "ForPhiDown.pt";

        torch::load(tagger[n], std::string(std::getenv("CHDIR")) + "/DNN/Model/htagger" + taggerName);
        tagger[n]->to(device);

        //Do the prediction
        int batchSize = 1024;
        int nBatches = tIndex.size(0) % batchSize == 0 ? tIndex.size(0)/batchSize -1 : std::ceil(tIndex.size(0)/batchSize);

        std::vector<torch::Tensor> predictions;

        for(int i=0; i <= nBatches; i++){
            torch::NoGradGuard no_grad;

            torch::Tensor prediction = tagger[n]->forward(
                        charged.narrow(0, batchSize*i, i!=nBatches ? batchSize : tIndex.size(0) - i*batchSize),
                        neutral.narrow(0, batchSize*i, i!=nBatches ? batchSize : tIndex.size(0) - i*batchSize),
                        SV.narrow(0, batchSize*i, i!=nBatches ? batchSize : tIndex.size(0) - i*batchSize), 
                        false);


            predictions.push_back(prediction);
        }

        torch::Tensor allPrediction = torch::cat(predictions, 0);

        //Fill Up/Down prediction to right position in the event
        for(int j=0; j < allPrediction.size(0); j++){
            tagValues[tIndex[j].item<long>()] = allPrediction[j].item<float>();
        }
    }

    return tagValues;
}

void TreeAppender::Append(){
    //Get old Tree
    TFile* oldF = TFile::Open(oldFile.c_str(), "READ");
    TTree* oldT = (TTree*)oldF->Get(oldTree.c_str());

    //Disables wished branches if they are already existing in old tree
    for(std::string& branchName: branchNames){
        oldT->SetBranchStatus(branchName.c_str(), 0);
    }

    //Clone Tree
    TFile* newF = TFile::Open(newFile.c_str(), "RECREATE");
    TTree* newT = oldT->CloneTree(0);
    newT->SetDirectory(newF);

    for(int i = entryStart; i < entryEnd; i++){
        oldT->GetEntry(i);
        newT->Fill();
    }

    if(false){
        return;
    }

    delete oldT;
    delete oldF;

    //Set new branches
    std::vector<float> branchValues(branchNames.size(), -999.);
    std::vector<std::vector<float>> values;
    std::vector<TBranch*> branches;

    for(unsigned int i=0; i < branchNames.size(); i++){
        if(branchNames[i] == "ML_HTagFJ1") values.push_back(HScore(0));
        if(branchNames[i] == "ML_HTagFJ2") values.push_back(HScore(1));

        TBranch* branch = newT->Branch(branchNames[i].c_str(), &branchValues[i]);
        branches.push_back(branch);
    }

    //Fill branches
    for(int i=0; i < newT->GetEntries(); i++){
        for(int j=0; j < branches.size(); j++){
            branchValues[j] = values[j][i];
            branches[j]->Fill();
        }
    }

    newF->cd();
    newT->Write();
    
    delete newT;
    delete newF;
}
